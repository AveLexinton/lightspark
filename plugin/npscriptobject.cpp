/**************************************************************************
    Lightspark, a free flash player implementation

    Copyright (C) 2010  Alessandro Pignotti (a.pignotti@sssup.it)
    Copyright (C) 2010  Timon Van Overveldt (timonvo@gmail.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include "logger.h"
#include "plugin.h"

#include "npscriptobject.h"

using namespace std;

/* -- NPIdentifierObject -- */
// Constructors
NPIdentifierObject::NPIdentifierObject(const std::string& value)
{
	stringToInt(value);
}
NPIdentifierObject::NPIdentifierObject(const char* value)
{
	stringToInt(std::string(value));
}
NPIdentifierObject::NPIdentifierObject(int32_t value)
{
	identifier = NPN_GetIntIdentifier(value);
}
NPIdentifierObject::NPIdentifierObject(const ExtIdentifier& value)
{
	// It is possible we got a down-casted ExtIdentifier, so lets check for that
	try
	{
		dynamic_cast<const NPIdentifierObject&>(value).copy(identifier);
	}
	// We got a real ExtIdentifier, lets convert it
	catch(std::bad_cast&)
	{
		if(value.getType() == EI_STRING)
			identifier = NPN_GetStringIdentifier(value.getString().c_str());
		else
			identifier = NPN_GetIntIdentifier(value.getInt());
	}
}
NPIdentifierObject::NPIdentifierObject(const NPIdentifierObject& id)
{
	id.copy(identifier);
}
NPIdentifierObject::NPIdentifierObject(const NPIdentifier& id)
{
	copy(id, identifier);
}

// Convert integer string identifiers to integer identifiers
void NPIdentifierObject::stringToInt(const std::string& value)
{
	char* endptr;
	int intValue = strtol(value.c_str(), &endptr, 10);
	
	// Convert integer string identifiers to integer identifiers
	if(*endptr == '\0')
		identifier = NPN_GetIntIdentifier(intValue);
	else
		identifier = NPN_GetStringIdentifier(value.c_str());
}

// Copy a given NPIdentifier into another one
void NPIdentifierObject::copy(const NPIdentifier& from, NPIdentifier& dest)
{
	if(NPN_IdentifierIsString(from))
		dest = NPN_GetStringIdentifier(NPN_UTF8FromIdentifier(from));
	else
		dest = NPN_GetIntIdentifier(NPN_IntFromIdentifier(from));
}

// Copy this object's value to a NPIdentifier
void NPIdentifierObject::copy(NPIdentifier& dest) const
{
	copy(identifier, dest);
}

// Comparator
bool NPIdentifierObject::operator<(const ExtIdentifier& other) const
{
	// It is possible we got a down-casted NPIdentifierObject, so lets check for that
	try
	{
		return identifier < dynamic_cast<const NPIdentifierObject&>(other).getNPIdentifier();
	}
	// We got a real ExtIdentifier, let ExtIdentifier::operator< handle this
	catch(std::bad_cast&)
	{
		return ExtIdentifier::operator<(other);
	}
}

// Type determination
NPIdentifierObject::EI_TYPE NPIdentifierObject::getType(const NPIdentifier& identifier)
{
	if(NPN_IdentifierIsString(identifier))
		return EI_STRING;
	else
		return EI_INT32;
}

// Conversion
std::string NPIdentifierObject::getString(const NPIdentifier& identifier)
{
	if(getType(identifier) == EI_STRING)
		return std::string(NPN_UTF8FromIdentifier(identifier));
	else
		return "";
}
int32_t NPIdentifierObject::getInt(const NPIdentifier& identifier)
{
	if(getType(identifier) == EI_INT32)
		return NPN_IntFromIdentifier(identifier);
	else
		return 0;
}
NPIdentifier NPIdentifierObject::getNPIdentifier() const
{
	if(getType() == EI_STRING) return NPN_GetStringIdentifier(getString().c_str());
	else return NPN_GetIntIdentifier(getInt());
}

/* -- NPObjectObject -- */
// Constructors
NPObjectObject::NPObjectObject() : instance(NULL)
{
}
NPObjectObject::NPObjectObject(NPP _instance) : instance(_instance)
{
}
NPObjectObject::NPObjectObject(NPP _instance, lightspark::ExtObject& other) : instance(_instance)
{
	setType(other.getType());
	copy(other, *this);
}
NPObjectObject::NPObjectObject(NPP _instance, NPObject* obj) :
	instance(_instance)
{
	NPIdentifier* ids = NULL;
	NPVariant property;
	uint32_t count;

	// Used to determine if the object we are converting is an array.
	bool allIntIds = true;

	if(instance == NULL || obj == NULL)
		return;

	// Get all identifiers this NPObject has
	if(NPN_Enumerate(instance, obj, &ids, &count))
	{
		for(uint32_t i = 0; i < count; i++)
		{
			if(NPN_GetProperty(instance, obj, ids[i], &property))
			{
				if(NPN_IdentifierIsString(ids[i]))
					allIntIds = false;

				setProperty(NPIdentifierObject(ids[i]), NPVariantObject(instance, property));
				NPN_ReleaseVariantValue(&property);
			}
		}

		// Arrays don't enumerate the length property. So if we only got integer ids and there exists
		// an un-enumerated length property of type integer, we actually got passed an array.
		if(allIntIds && NPN_GetProperty(instance, obj, NPN_GetStringIdentifier("length"), &property))
		{
			if(NPVariantObject::getType(property) == NPVariantObject::EV_INT32)
				setType(EO_ARRAY);

			NPN_ReleaseVariantValue(&property);
		}
		NPN_MemFree(ids);
	}
}

// Copying
NPObjectObject& NPObjectObject::operator=(const lightspark::ExtObject& other)
{
	setType(other.getType());
	copy(other, *this);
	return *this;
}
void NPObjectObject::copy(const lightspark::ExtObject& from, lightspark::ExtObject& to)
{
	lightspark::ExtIdentifier** ids;
	lightspark::ExtVariant* property;
	uint32_t count;
	if(from.enumerate(&ids, &count))
	{
		for(uint32_t i = 0; i < count; i++)
		{
			property = from.getProperty(*ids[i]);
			to.setProperty(*ids[i], *property);
			delete property;
			delete ids[i];
		}
	}
	delete ids;
}

// Properties
bool NPObjectObject::hasProperty(const lightspark::ExtIdentifier& id) const
{
	return properties.find(id) != properties.end();
}
lightspark::ExtVariant* NPObjectObject::getProperty(const lightspark::ExtIdentifier& id) const
{
	std::map<NPIdentifierObject, NPVariantObject>::const_iterator it = properties.find(id);
	if(it == properties.end())
		return NULL;

	return new NPVariantObject(instance, it->second);
}
void NPObjectObject::setProperty(const lightspark::ExtIdentifier& id, const lightspark::ExtVariant& value)
{
	properties[id] =  NPVariantObject(instance, value);
}

bool NPObjectObject::removeProperty(const lightspark::ExtIdentifier& id)
{
	std::map<NPIdentifierObject, NPVariantObject>::iterator it = properties.find(id);
	if(it == properties.end())
		return false;

	properties.erase(it);
	return true;
}
bool NPObjectObject::enumerate(lightspark::ExtIdentifier*** ids, uint32_t* count) const
{
	*count = properties.size();
	*ids = new lightspark::ExtIdentifier*[properties.size()];
	std::map<NPIdentifierObject, NPVariantObject>::const_iterator it;
	int i = 0;
	for(it = properties.begin(); it != properties.end(); ++it)
	{
		(*ids)[i] = new NPIdentifierObject(it->first);
		i++;
	}

	return true;
}

// Conversion to NPObject
NPObject* NPObjectObject::getNPObject(NPP instance, const lightspark::ExtObject& obj)
{
	uint32_t count = obj.getLength();

	// This will hold the enumerated properties
	lightspark::ExtVariant* property;
	// This will hold the converted properties
	NPVariant varProperty;

	NPObject* windowObject;
	NPN_GetValue(instance, NPNVWindowNPObject, &windowObject);

	// This will hold the result from NPN_Invoke
	NPVariant resultVariant;
	// This will hold the converted result from NPN_Invoke
	NPObject* result;

	// We are converting to a javascript Array
	if(obj.getType() == lightspark::ExtObject::EO_ARRAY)
	{
		// Create a new Array NPObject
		NPN_Invoke(instance, windowObject, NPN_GetStringIdentifier("Array"), NULL, 0, &resultVariant);
		// Convert our NPVariant result to an NPObject
		result = NPVARIANT_TO_OBJECT(resultVariant);

		// Push all values onto the array
		for(uint32_t i = 0; i < count; i++)
		{
			property = obj.getProperty(i);

			// The returned property might be an NPVariantObject, which would save us a conversion
			try
			{
				dynamic_cast<NPVariantObject&>(*property).copy(varProperty);
			}
			// We got a real ExtVariant, so we need to convert it to NPVariantObject first
			catch(std::bad_cast&)
			{
				NPVariantObject(instance, *property).copy(varProperty);
			}

			// Push the value onto the newly created Array
			NPN_Invoke(instance, result, NPN_GetStringIdentifier("push"), &varProperty, 1, &resultVariant);

			NPN_ReleaseVariantValue(&resultVariant);
			NPN_ReleaseVariantValue(&varProperty);
			delete property;
		}
	}
	else
	{
		// Create a new Object NPObject
		NPN_Invoke(instance, windowObject, NPN_GetStringIdentifier("Object"), NULL, 0, &resultVariant);
		// Convert our NPVariant result to an NPObject
		result = NPVARIANT_TO_OBJECT(resultVariant);

		lightspark::ExtIdentifier** ids = NULL;
		// Set all values of the object
		if(obj.enumerate(&ids, &count))
		{
			for(uint32_t i = 0; i < count; i++)
			{
				property = obj.getProperty(*ids[i]);

				// The returned property might be an NPVariantObject, which would save us a conversion
				try
				{
					dynamic_cast<NPVariantObject&>(*property).copy(varProperty);
				}
				// We got a real ExtVariant, so we need to convert it to NPVariantObject first
				catch(std::bad_cast&)
				{
					NPVariantObject(instance, *property).copy(varProperty);
				}

				// Set the properties
				NPN_SetProperty(instance, result, NPIdentifierObject(*ids[i]).getNPIdentifier(), &varProperty);

				NPN_ReleaseVariantValue(&varProperty);
				delete property;
				delete ids[i];
			}
		}
		if(ids != NULL)
			delete ids;
	}
	return result;
}

/* -- NPVariantObject -- */
// Constructors
NPVariantObject::NPVariantObject() : instance(NULL)
{
	VOID_TO_NPVARIANT(variant);
}
NPVariantObject::NPVariantObject(NPP _instance) : instance(_instance)
{
	VOID_TO_NPVARIANT(variant);
}
NPVariantObject::NPVariantObject(NPP _instance, const std::string& value) : instance(_instance)
{
	NPVariant temp;
	STRINGN_TO_NPVARIANT(value.c_str(), (int) value.size(), temp);
	copy(temp, variant); // Copy to make sure we own the data
}
NPVariantObject::NPVariantObject(NPP _instance, const char* value) : instance(_instance)
{
	NPVariant temp;
	STRINGN_TO_NPVARIANT(value, (int) strlen(value), temp);
	copy(temp, variant); // Copy to make sure we own the data
}
NPVariantObject::NPVariantObject(NPP _instance, int32_t value) : instance(_instance)
{
	INT32_TO_NPVARIANT(value, variant);
}
NPVariantObject::NPVariantObject(NPP _instance, double value) : instance(_instance)
{
	DOUBLE_TO_NPVARIANT(value, variant);
}
NPVariantObject::NPVariantObject(NPP _instance, bool value) : instance(_instance)
{
	BOOLEAN_TO_NPVARIANT(value, variant);
}

NPVariantObject::NPVariantObject(NPP _instance, const ExtVariant& value) : instance(_instance)
{
	// It's possible we got a down-casted NPVariantObject, so lets check for it
	try
	{
		dynamic_cast<const NPVariantObject&>(value).copy(variant);
	}
	// Seems we got a real ExtVariant, lets convert
	catch(std::bad_cast&)
	{
		switch(value.getType())
		{
		case EV_STRING:
			{
				std::string strValue = value.getString();
				NPVariant temp;
				STRINGN_TO_NPVARIANT(strValue.c_str(), (int) strValue.size(), temp);
				copy(temp, variant); // Copy to make sure we own the data
				break;
			}
		case EV_INT32:
			INT32_TO_NPVARIANT(value.getInt(), variant);
			break;
		case EV_DOUBLE:
			DOUBLE_TO_NPVARIANT(value.getDouble(), variant);
			break;
		case EV_BOOLEAN:
			BOOLEAN_TO_NPVARIANT(value.getBoolean(), variant);
			break;
		case EV_OBJECT:
			{
				lightspark::ExtObject* obj = value.getObject();
				OBJECT_TO_NPVARIANT(NPObjectObject::getNPObject(instance, *obj), variant);
				delete obj;
				break;
			}
		case EV_NULL:
			NULL_TO_NPVARIANT(variant);
			break;
		case EV_VOID:
		default:
			VOID_TO_NPVARIANT(variant);
			break;
		}
	}
}
NPVariantObject::NPVariantObject(NPP _instance, const NPVariantObject& other) : instance(_instance)
{
	other.copy(variant);
}
NPVariantObject::NPVariantObject(NPP _instance, const NPVariant& other) : instance(_instance)
{
	copy(other, variant);
}

// Destructor
NPVariantObject::~NPVariantObject()
{
	NPN_ReleaseVariantValue(&variant);
}

// Assignment operator
NPVariantObject& NPVariantObject::operator=(const NPVariantObject& other)
{
	instance = other.getInstance();
	if(this != &other)
	{
		NPN_ReleaseVariantValue(&variant);
		other.copy(variant);
	}
	return *this;
}
NPVariantObject& NPVariantObject::operator=(const NPVariant& other)
{
	if(&variant != &other)
	{
		NPN_ReleaseVariantValue(&variant);
		copy(other, variant);
	}
	return *this;
}

// Type determination
NPVariantObject::EV_TYPE NPVariantObject::getType(const NPVariant& variant)
{
	if(NPVARIANT_IS_STRING(variant))
		return EV_STRING;
	else if(NPVARIANT_IS_INT32(variant))
		return EV_INT32;
	else if(NPVARIANT_IS_DOUBLE(variant))
		return EV_DOUBLE;
	else if(NPVARIANT_IS_BOOLEAN(variant))
		return EV_BOOLEAN;
	else if(NPVARIANT_IS_OBJECT(variant))
		return EV_OBJECT;
	else if(NPVARIANT_IS_NULL(variant))
		return EV_NULL;
	else
		return EV_VOID;
}

// Conversion
std::string NPVariantObject::getString(const NPVariant& variant)
{
	if(getType(variant) == EV_STRING)
	{
		NPString val = NPVARIANT_TO_STRING(variant);
		return std::string(val.UTF8Characters, val.UTF8Length);
	}
	return "";
}
int32_t NPVariantObject::getInt(const NPVariant& variant)
{
	if(getType(variant) == EV_INT32)
		return NPVARIANT_TO_INT32(variant);
	return 0;
}
double NPVariantObject::getDouble(const NPVariant& variant)
{
	if(getType(variant) == EV_DOUBLE)
		return NPVARIANT_TO_DOUBLE(variant);
	return 0.0;
}
bool NPVariantObject::getBoolean(const NPVariant& variant)
{
	if(getType(variant) == EV_BOOLEAN)
		return NPVARIANT_TO_BOOLEAN(variant);
	return false;
}
lightspark::ExtObject* NPVariantObject::getObject() const
{
	return new NPObjectObject(instance, NPVARIANT_TO_OBJECT(variant));
}

// Copying
void NPVariantObject::copy(const NPVariant& from, NPVariant& dest)
{
	dest = from;

	switch(from.type)
	{
	case NPVariantType_String:
		{
			const NPString& value = NPVARIANT_TO_STRING(from);
			
			NPUTF8* newValue = static_cast<NPUTF8*>(NPN_MemAlloc(value.UTF8Length));
			memcpy(newValue, value.UTF8Characters, value.UTF8Length);

			STRINGN_TO_NPVARIANT(newValue, value.UTF8Length, dest);
			break;
		}
	case NPVariantType_Object:
		NPN_RetainObject(NPVARIANT_TO_OBJECT(dest));
		break;
	default: {}
	}
}

/* -- NPScriptObject -- */
// Constructor
NPScriptObject::NPScriptObject(NPScriptObjectGW* _gw) :
	gw(_gw), instance(gw->getInstance()), shuttingDown(false), marshallExceptions(false)
{
	// This object is always created in the main plugin thread, so lets save
	// so that we can check if we are in the main plugin thread later on.
	mainThread = pthread_self();

	// Only one external call can be made at a time
	sem_init(&mutex, 0, 1);

	setProperty("$version", "10,0,r"SHORTVERSION);

	// Standard methods
	setMethod("SetVariable", new lightspark::ExtBuiltinCallback(stdSetVariable));
	setMethod("GetVariable", new lightspark::ExtBuiltinCallback(stdGetVariable));
	setMethod("GotoFrame", new lightspark::ExtBuiltinCallback(stdGotoFrame));
	setMethod("IsPlaying", new lightspark::ExtBuiltinCallback(stdIsPlaying));
	setMethod("LoadMovie", new lightspark::ExtBuiltinCallback(stdLoadMovie));
	setMethod("Pan", new lightspark::ExtBuiltinCallback(stdPan));
	setMethod("PercentLoaded", new lightspark::ExtBuiltinCallback(stdPercentLoaded));
	setMethod("Play", new lightspark::ExtBuiltinCallback(stdPlay));
	setMethod("Rewind", new lightspark::ExtBuiltinCallback(stdRewind));
	setMethod("SetZoomRect", new lightspark::ExtBuiltinCallback(stdSetZoomRect));
	setMethod("StopPlay", new lightspark::ExtBuiltinCallback(stdStopPlay));
	setMethod("Zoom", new lightspark::ExtBuiltinCallback(stdZoom));
	setMethod("TotalFrames", new lightspark::ExtBuiltinCallback(stdTotalFrames));
}

// Destructors
NPScriptObject::~NPScriptObject()
{
	sem_destroy(&mutex);
	std::map<NPIdentifierObject, lightspark::ExtCallback*>::iterator meth_it = methods.begin();
	while(meth_it != methods.end())
	{
		delete (*meth_it).second;
		methods.erase(meth_it++);
	}
}
// Destruction preparator
void NPScriptObject::destroy()
{
	shuttingDown = true;

	// Signal all call statusses to continue and not wait for their respective external functions to return
	std::vector<sem_t*>::iterator it = callStatusses.begin();
	while(it != callStatusses.end())
	{
		sem_post(*it);
		++it;
	}
	sem_post(&mutex);
}

// NPRuntime interface: invoking methods
bool NPScriptObject::invoke(NPIdentifier id, const NPVariant* args, uint32_t argc, NPVariant* result)
{
	NPIdentifierObject objId(id);
	// Check if the method exists
	std::map<NPIdentifierObject, lightspark::ExtCallback*>::iterator it;
	it = methods.find(objId);
	if(it == methods.end())
		return false;

	// Convert raw arguments to objects
	const lightspark::ExtVariant* objArgs[argc];
	for(uint32_t i = 0; i < argc; i++)
		objArgs[i] = new NPVariantObject(instance, args[i]);

	// Run the function
	lightspark::ExtVariant* objResult = NULL;
	bool res = it->second->call(*this, objId, objArgs, argc, &objResult);

	// Delete converted arguments
	for(uint32_t i = 0; i < argc; i++)
		delete objArgs[i];

	if(objResult != NULL)
	{
		// Copy the result into the raw NPVariant result and delete intermediate result
		NPVariantObject(instance, *objResult).copy(*result);
		delete objResult;
	}

	return res;
}

bool NPScriptObject::invokeDefault(const NPVariant* args, uint32_t argc, NPVariant* result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObjectGW::invokeDefault");
	return false;
}

// ExtScriptObject interface: methods
bool NPScriptObject::removeMethod(const lightspark::ExtIdentifier& id)
{
	std::map<NPIdentifierObject, lightspark::ExtCallback*>::iterator it = methods.find(id);
	if(it == methods.end())
		return false;

	methods.erase(it);
	return true;
}

// ExtScriptObject interface: properties
NPVariantObject* NPScriptObject::getProperty(const lightspark::ExtIdentifier& id) const
{
	std::map<NPIdentifierObject, NPVariantObject>::const_iterator it = properties.find(id);
	if(it == properties.end())
		return NULL;

	NPVariantObject* result = new NPVariantObject(instance, it->second);
	return result;
}
bool NPScriptObject::removeProperty(const lightspark::ExtIdentifier& id)
{
	std::map<NPIdentifierObject, NPVariantObject>::iterator it = properties.find(id);
	if(it == properties.end())
		return false;

	properties.erase(it);
	return true;
}

// ExtScriptObject interface: enumeration
bool NPScriptObject::enumerate(lightspark::ExtIdentifier*** ids, uint32_t* count) const
{
	*count = properties.size()+methods.size();
	*ids = new lightspark::ExtIdentifier*[properties.size()+methods.size()];
	std::map<NPIdentifierObject, NPVariantObject>::const_iterator prop_it;
	int i = 0;
	for(prop_it = properties.begin(); prop_it != properties.end(); ++prop_it)
	{
		(*ids)[i] = new NPIdentifierObject(prop_it->first);
		i++;
	}
	std::map<NPIdentifierObject, lightspark::ExtCallback*>::const_iterator meth_it;
	for(meth_it = methods.begin(); meth_it != methods.end(); ++meth_it)
	{
		(*ids)[i] = new NPIdentifierObject(meth_it->first);
		i++;
	}

	return true;
}

// ExtScriptObject interface: calling external methods
bool NPScriptObject::callExternal(const lightspark::ExtIdentifier& id,
		const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result)
{
	// Make sure we are the only external call being executed
	sem_wait(&mutex);
	if(shuttingDown)
	{
		// Forward the shutdown cascade
		sem_post(&mutex);
		return false;
	}

	// This will hold the result from our NPN_Invoke call
	NPVariant resultVariant;

	// These will get passed as arguments to NPN_Invoke
	NPVariant variantArgs[argc];
	for(uint32_t i = 0; i < argc; i++)
		NPVariantObject(instance, *args[i]).copy(variantArgs[i]);

	// Signals when the async has been completed
	// True if NPN_Invoke succeeded
	bool success = false;

	// Used to signal completion of asynchronous external call
	sem_t callStatus;
	sem_init(&callStatus, 0, 0);
	// Add this callStatus semaphore to the list of running call statusses to be cleaned up on shutdown
	callStatusses.push_back(&callStatus);
	EXT_CALL_DATA data = {
		&mainThread,
		instance,
		NPIdentifierObject(id).getNPIdentifier(),
		("(" + id.getString() + ")").c_str(),
		variantArgs,
		argc,
		&resultVariant,
		&callStatus,
		&success
	};

	// Called JS may invoke a callback, which in turn may invoke another external method, which needs this mutex
	sem_post(&mutex);

	// We are not in the main thread, so ask the browser to call this method asynchronously
	if(!pthread_equal(pthread_self(), mainThread))
		NPN_PluginThreadAsyncCall(instance, &callExternal, &data);
	// We are in the main thread, so we can call the method ourselves synchronously
	else
		callExternal(&data);

	// Wait for the (possibly asynchronously) called function to finish
	sem_wait(&callStatus);
	// This call status doesn't need to be cleaned up anymore on shutdown
	callStatusses.pop_back();
	sem_destroy(&callStatus);

	// Only after our called function has finished, do we ask for the mutex again to continue
	sem_wait(&mutex);

	// If NPN_Invoke succeeded, convert & copy its result and release it.
	if(success)
	{
		*result = new NPVariantObject(instance, resultVariant);
		NPN_ReleaseVariantValue(&resultVariant);
	}

	// Release the converted arguments
	for(uint32_t i = 0; i < argc; i++)
		NPN_ReleaseVariantValue(&variantArgs[i]);

	sem_post(&mutex);
	return success;
}
void NPScriptObject::callExternal(void* d)
{
	EXT_CALL_DATA* data = static_cast<EXT_CALL_DATA*>(d);

	// Assert we are in the main plugin thread
	assert(pthread_equal(pthread_self(), *data->mainThread));

	NPObject* windowObject;
	NPN_GetValue(data->instance, NPNVWindowNPObject, &windowObject);

	// Lets try to invoke the identifier as if it is a function name
	*(data->success) = NPN_Invoke(data->instance, windowObject, data->id,
				data->args, data->argc, data->result);
	// Simple invoke succeeded, no further tries needed
	if(*(data->success))
	{
		sem_post(data->callStatus);
		return;
	}

	// Simple invoke failed, now try to evaluate the "identifier" as a script
	NPString script;
	script.UTF8Characters = data->scriptString;
	script.UTF8Length = strlen(data->scriptString);
	*(data->success) = NPN_Evaluate(data->instance, windowObject, &script, data->result);

	// Evaluate failed or it didn't return an object
	if(!*(data->success) || !NPVARIANT_IS_OBJECT(*(data->result)))
	{
		sem_post(data->callStatus);
		return;
	}

	// Evaluate didn't fail AND returned an object, lets try to invoke this object
	NPVariant evalResult = *(data->result);
	NPObject* evalObj = NPVARIANT_TO_OBJECT(*(data->result));
	bool evalSuccess = *(data->success);
	// Lets try to invoke the default function on the object returned by the evaluation
	*(data->success) = NPN_InvokeDefault(data->instance, evalObj,
				data->args, data->argc, data->result);
	// Invocation of default function failed, it seems the evaluation returned a "normal" object.
	// Restore previous results.
	if(!*(data->success))
	{
		*(data->result) = evalResult;
		*(data->success) = evalSuccess;
	}
	// Invocation of the default function succeeded, lets release the intermediate object.
	else
		NPN_ReleaseVariantValue(&evalResult);

	sem_post(data->callStatus);
}

void NPScriptObject::setException(const std::string& message) const
{
	if(marshallExceptions)
		NPN_SetException(gw, message.c_str());
}

// Standard Flash methods
bool NPScriptObject::stdGetVariable(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdGetVariable");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance);
	return false;
}

bool NPScriptObject::stdSetVariable(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdSetVariable");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, false);
	return false;
}

bool NPScriptObject::stdGotoFrame(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdGotoFrame");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, false);
	return false;
}

bool NPScriptObject::stdIsPlaying(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdIsPlaying");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, true);
	return true;
}

bool NPScriptObject::stdLoadMovie(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdLoadMovie");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, false);
	return false;
}

bool NPScriptObject::stdPan(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdPan");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, false);
	return false;
}

bool NPScriptObject::stdPercentLoaded(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdPercentLoaded");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, 100);
	return true;
}

bool NPScriptObject::stdPlay(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdPlay");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, false);
	return false;
}

bool NPScriptObject::stdRewind(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdRewind");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, false);
	return false;
}

bool NPScriptObject::stdStopPlay(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdStopPlay");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, false);
	return false;
}

bool NPScriptObject::stdSetZoomRect(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdSetZoomRect");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, false);
	return false;
}

bool NPScriptObject::stdZoom(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdZoom");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, false);
	return false;
}

bool NPScriptObject::stdTotalFrames(const lightspark::ExtScriptObject& so,
			const lightspark::ExtIdentifier& id,
			const lightspark::ExtVariant** args, uint32_t argc, lightspark::ExtVariant** result)
{
	LOG(LOG_NOT_IMPLEMENTED, "NPScriptObject::stdTotalFrames");
	*result = new NPVariantObject(dynamic_cast<const NPScriptObject&>(so).instance, false);
	return false;
}


/* -- NPScriptObjectGW -- */
// NPScriptObjectGW NPClass for use with NPRuntime
NPClass NPScriptObjectGW::npClass = 
{
	NP_CLASS_STRUCT_VERSION,
	allocate,
	deallocate,
	invalidate,
	hasMethod,
	invoke,
	invokeDefault,
	hasProperty,
	getProperty,
	setProperty,
	removeProperty,
	enumerate,
	construct
};

// Constructor
NPScriptObjectGW::NPScriptObjectGW(NPP inst) : instance(inst)
{
	assert(instance != NULL);
	so = new NPScriptObject(this);

	NPN_GetValue(instance, NPNVWindowNPObject, &windowObject);
	NPN_GetValue(instance, NPNVPluginElementNPObject, &pluginElementObject);

	// Lets set these basic properties, fetched from the NPRuntime
	if(pluginElementObject != NULL)
	{
		NPVariant result;
		NPN_GetProperty(instance, pluginElementObject, NPN_GetStringIdentifier("id"), &result);
		NPVariantObject objResult(instance, result);
		so->setProperty("id", objResult);
		NPN_ReleaseVariantValue(&result);
		NPN_GetProperty(instance, pluginElementObject, NPN_GetStringIdentifier("name"), &result);
		so->setProperty("name", NPVariantObject(instance, result));
		NPN_ReleaseVariantValue(&result);
	}
}

// Destructor
NPScriptObjectGW::~NPScriptObjectGW()
{
	delete so;
};

// Properties
bool NPScriptObjectGW::getProperty(NPObject* obj, NPIdentifier id, NPVariant* result)
{
	sys = ((NPScriptObjectGW*) obj)->m_sys;
	
	NPVariantObject* resultObj = ((NPScriptObjectGW*) obj)->so->getProperty(NPIdentifierObject(id));
	if(resultObj == NULL)
	{
		sys = NULL;
		return false;
	}

	resultObj->copy(*result);
	delete resultObj;

	sys = NULL;
	return true;
}

// Enumeration
bool NPScriptObjectGW::enumerate(NPObject* obj, NPIdentifier** value, uint32_t* count)
{
	sys = ((NPScriptObjectGW*) obj)->m_sys;

	NPScriptObject* o = ((NPScriptObjectGW*) obj)->so;
	lightspark::ExtIdentifier** ids = NULL;
	bool success = o->enumerate(&ids, count);
	if(success)
	{
		*value = (NPIdentifier*) NPN_MemAlloc(sizeof(NPIdentifier)*(*count));
		for(uint32_t i = 0; i < *count; i++)
		{
			(*value)[i] = dynamic_cast<NPIdentifierObject&>(*ids[i]).getNPIdentifier();
			delete ids[i];
		}
	}

	if(ids != NULL)
		delete ids;

	sys = NULL;
	return success;
}
