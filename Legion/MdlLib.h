#pragma once

#include "StringBase.h"
#include "ListBase.h"
#include "DictionaryBase.h"
#include "MemoryStream.h"
#include "FileStream.h"
#include "BinaryReader.h"

#include "RpakAssets.h"
#include "ApexAsset.h"

#include "Model.h"
#include "Exporter.h"
#include "RpakLib.h"

// A class that handles reading assets from a respawn Vpk
class MdlLib
{
public:
	MdlLib();
	~MdlLib() = default;

	// The exporter formats for models and anims
	std::unique_ptr<Assets::Exporters::Exporter> ModelExporter;
	std::unique_ptr<Assets::Exporters::Exporter> AnimExporter;

	// Initializes a model exporter
	void InitializeModelExporter(ModelExportFormat_t Format = ModelExportFormat_t::SEModel);
	// Initializes a anim exporter
	void InitializeAnimExporter(AnimExportFormat_t Format = AnimExportFormat_t::SEAnim);

	// gets valve vertex data from model
	void ExtractValveVertexData(titanfall2::studiohdr_t* pHdr, vtx::FileHeader_t* pVtx, vvd::vertexFileHeader_t* pVVD, vvc::vertexColorFileHeader_t* pVVC, vvw::vertexBoneWeightsExtraFileHeader_t* pVVW,
		std::unique_ptr<Assets::Model> &ExportModel, const string& Path);

	void ExtractAnimValue(int frame, mstudioanimvalue_t* panimvalue, float scale, float& v1, float& v2);
	void ExtractAnimValue(int frame, mstudioanimvalue_t* panimvalue, float scale, float& v1);

	void CalcBoneQuaternion(int frame, float s,
							const Quaternion& baseQuat, const RadianEuler& baseRot, const Vector3& baseRotScale,
							int iBaseFlags, const Quaternion& baseAlignment,
							const titanfall2::mstudio_rle_anim_t* panim, Quaternion& q);
	inline void CalcBoneQuaternion(int frame, float s,
								const titanfall2::mstudiobone_t* pBone,
								const titanfall2::mstudiolinearbone_t* pLinearBones,
								const titanfall2::mstudio_rle_anim_t* panim, Quaternion& q);

	void CalcBonePosition(int frame, float s,
						const Vector3& basePos, const float& baseBoneScale,
						const titanfall2::mstudio_rle_anim_t* panim, Vector3& pos);
	inline void CalcBonePosition(int frame, float s,
								const titanfall2::mstudiobone_t* pBone,
								const titanfall2::mstudiolinearbone_t* pLinearBones,
								const titanfall2::mstudio_rle_anim_t* panim, Vector3& pos);

	void CalcBoneScale(int frame, float s,
					const Vector3& baseScale, Vector3& baseScaleScale,
					const  titanfall2::mstudio_rle_anim_t* panim, Vector3& scale);

	inline titanfall2::mstudio_rle_anim_t* pAnim(titanfall2::mstudioanimdesc_t* pAnimDesc, int* piFrame);

	// Exports an on-disk mdl asset
	void ExportMDLv53(const string& Asset, const string& Path);

//private:
	/*void ParseRAnimBoneTranslationTrack(const RAnimBoneHeader& BoneFlags, const titanfall2::mstudiobone_t& Bone, uint16_t** BoneTrackData, const std::unique_ptr<Assets::Animation>& Anim, uint32_t BoneIndex, uint32_t Frame, uint32_t FrameIndex);
	void ParseRAnimBoneRotationTrack(const RAnimBoneHeader& BoneFlags, const titanfall2::mstudiobone_t& Bone, uint16_t** BoneTrackData, const std::unique_ptr<Assets::Animation>& Anim, uint32_t BoneIndex, uint32_t Frame, uint32_t FrameIndex);
	void ParseRAnimBoneScaleTrack(const RAnimBoneHeader& BoneFlags, uint16_t** BoneTrackData, const std::unique_ptr<Assets::Animation>& Anim, uint32_t BoneIndex, uint32_t Frame, uint32_t FrameIndex);*/
};