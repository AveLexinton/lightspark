#include "flashdisplay3dtextures.h"
#include "scripting/class.h"
#include "scripting/argconv.h"
#include "scripting/flash/display/BitmapData.h"

namespace lightspark
{

void TextureBase::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, ASObject, CLASS_SEALED);
	c->setDeclaredMethodByQName("dispose","",Class<IFunction>::getFunction(c->getSystemState(),dispose),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(TextureBase,dispose)
{
	LOG(LOG_NOT_IMPLEMENTED,"TextureBase.dispose does nothing");
}

void Texture::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, TextureBase, CLASS_SEALED);
	c->setDeclaredMethodByQName("uploadCompressedTextureFromByteArray","",Class<IFunction>::getFunction(c->getSystemState(),uploadCompressedTextureFromByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromByteArray","",Class<IFunction>::getFunction(c->getSystemState(),uploadFromByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromBitmapData","",Class<IFunction>::getFunction(c->getSystemState(),uploadFromBitmapData),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(Texture,uploadCompressedTextureFromByteArray)
{
	LOG(LOG_NOT_IMPLEMENTED,"Texture.uploadCompressedTextureFromByteArray does nothing");
	_NR<ByteArray> data;
	int32_t byteArrayOffset;
	bool async;
	ARG_UNPACK_ATOM(data)(byteArrayOffset)(async,false);
}
ASFUNCTIONBODY_ATOM(Texture,uploadFromBitmapData)
{
	Texture* th = asAtomHandler::as<Texture>(obj);
	uint32_t miplevel;
	_NR<BitmapData> source;
	ARG_UNPACK_ATOM(source)(miplevel,0);
	if (source.isNull())
		throwError<TypeError>(kNullArgumentError);
	th->needrefresh = true;
	if (miplevel > 0 && ((uint32_t)1<<(miplevel-1) > max(th->width,th->height)))
	{
		LOG(LOG_ERROR,"invalid miplevel:"<<miplevel<<" "<<(1<<(miplevel-1))<<" "<< th->width<<" "<<th->height);
		throwError<ArgumentError>(kInvalidArgumentError,"miplevel");
	}
	if (th->bitmaparray.size() <= miplevel)
		th->bitmaparray.resize(miplevel+1);
	uint32_t mipsize = (th->width>>miplevel)*(th->height>>miplevel)*4;
	th->bitmaparray[miplevel].resize(mipsize);

	for (uint32_t i = 0; i < (th->height>>miplevel); i++)
	{
		for (uint32_t j = 0; j < (th->width>>miplevel); j++)
		{
			// It seems that flash expects the bitmaps to be premultiplied-alpha in shaders
			uint8_t alpha = source->getBitmapContainer()->getData()[i*source->getBitmapContainer()->getWidth()*4 + j*4+3];
			th->bitmaparray[miplevel][i*(th->width>>miplevel)*4 + j*4] = (uint8_t)(source->getBitmapContainer()->getData()[i*source->getBitmapContainer()->getWidth()*4 + j*4]*alpha /255);
			th->bitmaparray[miplevel][i*(th->width>>miplevel)*4 + j*4+1] = (uint8_t)(source->getBitmapContainer()->getData()[i*source->getBitmapContainer()->getWidth()*4 + j*4+1]*alpha /255);
			th->bitmaparray[miplevel][i*(th->width>>miplevel)*4 + j*4+2] = (uint8_t)(source->getBitmapContainer()->getData()[i*source->getBitmapContainer()->getWidth()*4 + j*4+2]*alpha /255);
			th->bitmaparray[miplevel][i*(th->width>>miplevel)*4 + j*4+3] = source->getBitmapContainer()->getData()[i*source->getBitmapContainer()->getWidth()*4 + j*4+3];
		}
	}
	th->context->addAction(RENDER_LOADTEXTURE,th);
}
ASFUNCTIONBODY_ATOM(Texture,uploadFromByteArray)
{
	LOG(LOG_NOT_IMPLEMENTED,"Texture.uploadFromByteArray does nothing");
	_NR<ByteArray> data;
	int32_t byteArrayOffset;
	uint32_t miplevel;
	ARG_UNPACK_ATOM(data)(byteArrayOffset)(miplevel,0);
}


void CubeTexture::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, TextureBase, CLASS_SEALED);
	c->setDeclaredMethodByQName("uploadCompressedTextureFromByteArray","",Class<IFunction>::getFunction(c->getSystemState(),uploadCompressedTextureFromByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromByteArray","",Class<IFunction>::getFunction(c->getSystemState(),uploadFromByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromBitmapData","",Class<IFunction>::getFunction(c->getSystemState(),uploadFromBitmapData),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(CubeTexture,uploadCompressedTextureFromByteArray)
{
	LOG(LOG_NOT_IMPLEMENTED,"CubeTexture.uploadCompressedTextureFromByteArray does nothing");
	_NR<ByteArray> data;
	int32_t byteArrayOffset;
	bool async;
	ARG_UNPACK_ATOM(data)(byteArrayOffset)(async,false);
}
ASFUNCTIONBODY_ATOM(CubeTexture,uploadFromBitmapData)
{
	CubeTexture* th = asAtomHandler::as<CubeTexture>(obj);
	_NR<BitmapData> source;
	uint32_t side;
	uint32_t miplevel;
	ARG_UNPACK_ATOM(source)(side)(miplevel,0);
	if (source.isNull())
		throwError<TypeError>(kNullArgumentError);
	th->needrefresh = true;
	if (miplevel > 0 && ((uint32_t)1<<(miplevel-1) > th->width))
	{
		LOG(LOG_ERROR,"invalid miplevel:"<<miplevel<<" "<<(1<<(miplevel-1))<<" "<< th->width);
		throwError<ArgumentError>(kInvalidArgumentError,"miplevel");
	}
	if (side > 5)
	{
		LOG(LOG_ERROR,"invalid side:"<<side);
		throwError<ArgumentError>(kInvalidArgumentError,"side");
	}
	uint32_t bitmap_size = 1<<(th->max_miplevel-(miplevel+1));
	if ((source->getWidth() != source->getHeight())
		|| (source->getWidth() != bitmap_size))
	{
		LOG(LOG_ERROR,"invalid bitmap:"<<source->getWidth()<<" "<<source->getHeight()<<" "<< th->max_miplevel <<" "<< miplevel);
		throwError<ArgumentError>(kInvalidArgumentError,"source");
	}

	uint32_t mipsize = (th->width>>miplevel)*(th->width>>miplevel)*4;
	th->bitmaparray[th->max_miplevel*side + miplevel].resize(mipsize);
	for (uint32_t i = 0; i < bitmap_size; i++)
	{
		for (uint32_t j = 0; j < bitmap_size; j++)
		{
			// It seems that flash expects the bitmaps to be premultiplied-alpha in shaders
			uint8_t alpha = source->getBitmapContainer()->getData()[i*source->getBitmapContainer()->getWidth()*4 + j*4+3];
			th->bitmaparray[miplevel][i*(th->width>>miplevel)*4 + j*4] = (uint8_t)(source->getBitmapContainer()->getData()[i*source->getBitmapContainer()->getWidth()*4 + j*4]*alpha /255);
			th->bitmaparray[miplevel][i*(th->width>>miplevel)*4 + j*4+1] = (uint8_t)(source->getBitmapContainer()->getData()[i*source->getBitmapContainer()->getWidth()*4 + j*4+1]*alpha /255);
			th->bitmaparray[miplevel][i*(th->width>>miplevel)*4 + j*4+2] = (uint8_t)(source->getBitmapContainer()->getData()[i*source->getBitmapContainer()->getWidth()*4 + j*4+2]*alpha /255);
			th->bitmaparray[miplevel][i*(th->width>>miplevel)*4 + j*4+3] = source->getBitmapContainer()->getData()[i*source->getBitmapContainer()->getWidth()*4 + j*4+3];
		}
	}
	th->context->addAction(RENDER_LOADCUBETEXTURE,th);
}
ASFUNCTIONBODY_ATOM(CubeTexture,uploadFromByteArray)
{
	LOG(LOG_NOT_IMPLEMENTED,"CubeTexture.uploadFromByteArray does nothing");
	_NR<ByteArray> data;
	int32_t byteArrayOffset;
	uint32_t side;
	uint32_t miplevel;
	ARG_UNPACK_ATOM(data)(byteArrayOffset)(side)(miplevel,0);
}

void RectangleTexture::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, TextureBase, CLASS_SEALED);
	c->setDeclaredMethodByQName("uploadFromByteArray","",Class<IFunction>::getFunction(c->getSystemState(),uploadFromByteArray),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("uploadFromBitmapData","",Class<IFunction>::getFunction(c->getSystemState(),uploadFromBitmapData),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_ATOM(RectangleTexture,uploadFromBitmapData)
{
	LOG(LOG_NOT_IMPLEMENTED,"RectangleTexture.uploadFromBitmapData does nothing");
	_NR<BitmapData> source;
	ARG_UNPACK_ATOM(source);
}
ASFUNCTIONBODY_ATOM(RectangleTexture,uploadFromByteArray)
{
	LOG(LOG_NOT_IMPLEMENTED,"RectangleTexture.uploadFromByteArray does nothing");
	_NR<ByteArray> data;
	int32_t byteArrayOffset;
	ARG_UNPACK_ATOM(data)(byteArrayOffset);
}

void VideoTexture::sinit(Class_base *c)
{
	CLASS_SETUP_NO_CONSTRUCTOR(c, TextureBase, CLASS_SEALED);
	REGISTER_GETTER(c,videoHeight);
	REGISTER_GETTER(c,videoWidth);
	c->setDeclaredMethodByQName("attachCamera","",Class<IFunction>::getFunction(c->getSystemState(),attachCamera),NORMAL_METHOD,true);
	c->setDeclaredMethodByQName("attachNetStream","",Class<IFunction>::getFunction(c->getSystemState(),attachNetStream),NORMAL_METHOD,true);
}
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(VideoTexture,videoHeight);
ASFUNCTIONBODY_GETTER_NOT_IMPLEMENTED(VideoTexture,videoWidth);

ASFUNCTIONBODY_ATOM(VideoTexture,attachCamera)
{
	LOG(LOG_NOT_IMPLEMENTED,"VideoTexture.attachCamera does nothing");
//	_NR<Camera> theCamera;
//	ARG_UNPACK_ATOM(theCamera);
}
ASFUNCTIONBODY_ATOM(VideoTexture,attachNetStream)
{
	LOG(LOG_NOT_IMPLEMENTED,"VideoTexture.attachNetStream does nothing");
	_NR<NetStream> netStream;
	ARG_UNPACK_ATOM(netStream);
}

}
