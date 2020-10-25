#pragma once
#pragma unmanaged
#include "stdafx.h"

namespace extras
{
extern char* partNames[];
extern char* surfaceNames[];
std::string GetShortFilename(const char* LongFilename, std::string* path= NULL);
std::string GetShortFilename(const std::string& LongFilename, std::string* path= NULL);
std::string _stdcall GetRscString(UINT resourceID);

template <typename Z> std::string ToString(const Z& input)
{
	std::stringstream buffer;
	buffer << input;
	return buffer.str();
}

template<typename CustomObj>inline CustomObj GetObjectFromNode(INode* leNode, TimeValue t, const UINT& classIDA, int& deleteIt)
{
	deleteIt = FALSE;    
	Object *obj = leNode->EvalWorldState(t).obj;    
	if (obj->CanConvertToType(Class_ID(classIDA, 0))) 
	{        
		CustomObj custObj = (CustomObj) obj->ConvertToType(t, Class_ID(classIDA, 0));                
		// Note that the TriObject should only be deleted        
		// if the pointer to it is not equal to the object        
		// pointer that called ConvertToType()        
		if (obj != custObj) 
			deleteIt = TRUE;             
		return custObj;        
	}        
	else 
	{             
		return NULL;        
	}
}

#pragma pack(1)
enum FileVersions : UINT { GTA_SA= 0x1803ffff };
enum secIDs: UINT {ATOMIC= 0x14, CLUMP_ID= 0x10, HANIM_PLG= 0x11e, FRAME= 0x253f2fe, FRAME_LIST= 0x0E, GEOMETRY_LIST= 0x1a, EXTENSION= 0x03, MATERIAL= 0x07, MATERIAL_LIST= 0x08
					,TEXTURE= 0x06, GEOMETRY= 0x0F, STRUCT= 0x01, MATERIAL_EFFECTS_PLG= 0x120, REFLECTION_MATERIAL= 0x253f2fc,SPECULAR_MATERIAL= 0x253f2f6 
					,BIN_MESH_PLG= 0x50e, NATIVE_DATA_PLG= 0x0510, SKIN_PLG= 0x0116, MESH_EXTENSION= 0x0253F2FD, NIGHT_VERTEX_COLORS= 0x0253F2F9
					,MORPH_PLG= 0x0105, TWO_DFX_PLG= 0x0253F2F8, UVANIM_DIC= 0x2B, ANIM_ANIMATION= 0x1B, UV_ANIM_PLG= 0x135, RIGHT_TO_RENDER= 0x1f
					,STRING= 0x02, LIGHT= 0x12, COLLISION= 0x253f2fa, PIPELINE= 0x253f2f3, ZMOD_LOCK= 0xf21e, CAMERA= 0x05};
enum ColIDs : UINT {COLL= 0x4c4c4f43, COL2= 0x324c4f43, COL3= 0x334c4f43};
struct TBoundsV1
{
	float BSphereRad;
	float BCenter[3];
	float BBoxMin[3];
	float BBoxMax[3];
};

struct TBoundsV23
{
	float BBoxMin[3];
	float BBoxMax[3];
	float BCenter[3];
	float BSphereRad;
};

struct ColHeader1_t
{
	union {
		UINT FourCC;
		char FourCCAsString[4];
	};
	UINT FileSizeMinus8;
	char CollisionName[20];
	char Unused[4];
	union {
		extras::TBoundsV1 boundsVer1;
		extras::TBoundsV23 boundsVer23;
	};

	bool CheckHeader()
	{
		return (FourCC == COLL || FourCC == COL2 || FourCC == COL3);
	}
};

struct ColHeader23_t
{
	USHORT numColSpheres;
	USHORT numColBoxes;
	UINT numColmeshFaces;
	UINT flags;
	UINT ColSphereOffset;
	UINT ColBoxOffset;
	UINT UnknownOffset1;
	UINT ColmeshVtxOffset;
	UINT ColmeshFaceOffset;
	UINT UnknownOffset2;
};

struct ColHeader3_t
{
	UINT numShadowmeshFaces;
	UINT ShadowmeshVtxOffset;
	UINT ShadowmeshFaceOffset;
};

struct TSurfaceV1
{
	BYTE material;
	BYTE flag;
	BYTE brightness;
	BYTE light;
};

struct TSurfaceV23
{
	BYTE material;
	BYTE light;
};

struct TBox
{
	float min[3];
	float max[3];
	union {
		TSurfaceV1 surface;
		UINT surfaceAsUINT;
	};
};

struct TSphereV1
{
	float radius;
	float center[3];
	union {
		TSurfaceV1 surface;
		UINT surfaceAsUINT;
	};
};

struct TSphereV23
{
	float center[3];
	float radius;
	union {
		TSurfaceV1 surface;
		UINT surfaceAsUINT;
	};
};

struct TFaceGroup
{
	float min[3], max[3];
	USHORT StartFace, EndFace;
};

struct TFaceV1
{
	UINT a,b,c;
	union {
		TSurfaceV1 surface;
		UINT surfaceCompressed;
	};

	UINT& operator [](const BYTE& input)
	{
		switch (input)
		{
			case 0:
				return a;
			break;

			case 1:
				return b;
			break;

			default:
				return c;
			break;
		}
	}
};

struct TFaceV23
{
	USHORT a,b,c;
	union {
		TSurfaceV23 surface;
		USHORT surfaceCompressed;
	};

	//returns a, b or c
	USHORT& operator [](const BYTE& input)
	{
		switch(input)
		{
			case 0:
				return a;
			break;

			case 1:
				return b;
			break;

			default:
				return c;
			break;
		}
	}
};

struct GTAHeader
{
	UINT headerID;
	UINT secSize;
	UINT fileVer;
};

#pragma pack()
template <typename T> struct customLess : std::binary_function<T, T, bool>
{
	bool operator () (const T& x, const T& y) const
	{
		return (x.surfaceAsUINT < y.surfaceAsUINT);
	};
};

class BinStream : public ::std::fstream
{
public:

	template <typename T> BinStream& operator << (const T& input)
	{
		T& temp= const_cast<T&>(input);
		this->write(reinterpret_cast<char*>(&temp), sizeof(T));
		return *this;
	}

	template <typename T> BinStream& operator >> (T& input)
	{
		this->read(reinterpret_cast<char*>(&input), sizeof(T));
		return *this;
	}

	template <typename T> T GenericRead ()
	{
		T temp;
		this->read(reinterpret_cast<char*>(&temp), sizeof(T));
		return temp;
	}
};
}