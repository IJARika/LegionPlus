#include "ExportManager.h"
#include "ParallelTask.h"
#include "Path.h"
#include "Directory.h"
#include "File.h"
#include "Environment.h"
#include <atomic>
#include <mutex>

System::Settings ExportManager::Config = System::Settings();
string ExportManager::ApplicationPath = "";
string ExportManager::ExportPath = "";

void ExportManager::InitializeExporter()
{
	ApplicationPath = System::Environment::GetApplicationPath();

	auto ConfigPath = IO::Path::Combine(ApplicationPath, "Legion.cfg");

	if (IO::File::Exists(ConfigPath))
	{
		Config.Load(ConfigPath);
	}

	if (!Config.Has<System::SettingType::String>("ExportDirectory"))
	{
		ExportPath = IO::Path::Combine(ApplicationPath, "exported_files");
	}
	else
	{
		auto ExportDirectory = Config.Get<System::SettingType::String>("ExportDirectory");

		if (IO::Directory::Exists(ExportDirectory))
		{
			ExportPath = ExportDirectory;
		}
		else
		{
			ExportPath = IO::Path::Combine(ApplicationPath, "exported_files");
			Config.Remove<System::SettingType::String>("ExportDirectory");
		}
	}

	if (!Config.Has<System::SettingType::Integer>("ModelFormat"))
		Config.Set<System::SettingType::Integer>("ModelFormat", (uint32_t)RpakModelExportFormat::SEModel);
	if (!Config.Has<System::SettingType::Integer>("AnimFormat"))
		Config.Set<System::SettingType::Integer>("AnimFormat", (uint32_t)RpakAnimExportFormat::SEAnim);
	if (!Config.Has<System::SettingType::Integer>("ImageFormat"))
		Config.Set<System::SettingType::Integer>("ImageFormat", (uint32_t)RpakImageExportFormat::Dds);

	if (!Config.Has<System::SettingType::Boolean>("LoadModels"))
		Config.Set<System::SettingType::Boolean>("LoadModels", true);
	if (!Config.Has<System::SettingType::Boolean>("LoadAnimations"))
		Config.Set<System::SettingType::Boolean>("LoadAnimations", true);
	if (!Config.Has<System::SettingType::Boolean>("LoadImages"))
		Config.Set<System::SettingType::Boolean>("LoadImages", true);
	if (!Config.Has<System::SettingType::Boolean>("LoadMaterials"))
		Config.Set<System::SettingType::Boolean>("LoadMaterials", true);
	if (!Config.Has<System::SettingType::Boolean>("LoadUIImages"))
		Config.Set<System::SettingType::Boolean>("LoadUIImages", true);
	if (!Config.Has<System::SettingType::Boolean>("LoadDataTables"))
		Config.Set<System::SettingType::Boolean>("LoadDataTables", true);
		
	Config.Save(ConfigPath);
}

void ExportManager::SaveConfigToDisk()
{
	auto ExportDirectory = Config.Get<System::SettingType::String>("ExportDirectory");

	if (IO::Directory::Exists(ExportDirectory))
	{
		ExportPath = ExportDirectory;
	}
	else
	{
		ExportPath = IO::Path::Combine(ApplicationPath, "exported_files");
		Config.Remove<System::SettingType::String>("ExportDirectory");
	}

	Config.Save(IO::Path::Combine(ApplicationPath, "Legion.cfg"));
}

string ExportManager::GetMapExportPath()
{
	auto Result = IO::Path::Combine(ExportPath, "maps");
	IO::Directory::CreateDirectory(Result);
	return Result;
}

void ExportManager::ExportMilesAssets(const std::unique_ptr<MilesLib>& MilesFileSystem, List<ExportAsset> ExportAssets, ExportProgressCallback ProgressCallback, CheckStatusCallback StatusCallback, Forms::Form* MainForm)
{
	std::atomic<uint32_t> AssetIndex = 0;

	std::mutex UpdateMutex;
	uint32_t CurrentProgress = 0;
	string ExportDirectory = ExportPath;

	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "sounds"));

	Threading::ParallelTask([&MilesFileSystem, &ExportAssets, &ProgressCallback, &StatusCallback, &MainForm, &AssetIndex, &CurrentProgress, &UpdateMutex, ExportDirectory]
	{
		bool IsCancel = false;

		while (AssetIndex < ExportAssets.Count() && !IsCancel)
		{
			auto AssetToConvert = AssetIndex++;

			if (AssetToConvert >= ExportAssets.Count())
				continue;

			auto& Asset = ExportAssets[AssetToConvert];
			auto& AudioAsset = MilesFileSystem->Assets[Asset.AssetHash];

			MilesFileSystem->ExtractAsset(AudioAsset, IO::Path::Combine(IO::Path::Combine(ExportDirectory, "sounds"), AudioAsset.Name + ".wav"));

			IsCancel = StatusCallback(Asset.AssetIndex, MainForm);

			{
				std::lock_guard<std::mutex> UpdateLock(UpdateMutex);
				auto NewProgress = (uint32_t)(((float)AssetToConvert / (float)ExportAssets.Count()) * 100.f);

				if (NewProgress > CurrentProgress)
				{
					CurrentProgress = NewProgress;
					ProgressCallback(NewProgress, MainForm, false);
				}
			}
		}
	});

	ProgressCallback(100, MainForm, true);
}

void ExportManager::ExportRpakAssets(const std::unique_ptr<RpakLib>& RpakFileSystem, List<ExportAsset> ExportAssets, ExportProgressCallback ProgressCallback, CheckStatusCallback StatusCallback, Forms::Form* MainForm)
{
	std::atomic<uint32_t> AssetIndex = 0;

	std::mutex UpdateMutex;
	uint32_t CurrentProgress = 0;
	string ExportDirectory = ExportPath;

	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "images"));
	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "materials"));
	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "models"));
	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "animations"));
	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "subtitles"));
	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "datatables"));

	RpakFileSystem->InitializeModelExporter((RpakModelExportFormat)Config.Get<System::SettingType::Integer>("ModelFormat"));
	RpakFileSystem->InitializeAnimExporter((RpakAnimExportFormat)Config.Get<System::SettingType::Integer>("AnimFormat"));
	RpakFileSystem->InitializeImageExporter((RpakImageExportFormat)Config.Get<System::SettingType::Integer>("ImageFormat"));

	Threading::ParallelTask([&RpakFileSystem, &ExportAssets, &ProgressCallback, &StatusCallback, &MainForm, &AssetIndex, &CurrentProgress, &UpdateMutex, ExportDirectory]
	{
		(void)CoInitializeEx(0, COINIT_MULTITHREADED);

		bool IsCancel = false;

		while (AssetIndex < ExportAssets.Count() && !IsCancel)
		{
			auto AssetToConvert = AssetIndex++;

			if (AssetToConvert >= ExportAssets.Count())
				continue;

			auto& Asset = ExportAssets[AssetToConvert];
			auto& AssetToExport = RpakFileSystem->Assets[Asset.AssetHash];

			switch (AssetToExport.AssetType)
			{
			case (uint32_t)RpakAssetType::Texture:
				RpakFileSystem->ExportTexture(AssetToExport, IO::Path::Combine(ExportDirectory, "images"), true);
				break;
			case (uint32_t)RpakAssetType::UIIA:
				RpakFileSystem->ExportUIIA(AssetToExport, IO::Path::Combine(ExportDirectory, "images"));
				break;
			case (uint32_t)RpakAssetType::Material:
				RpakFileSystem->ExportMaterial(AssetToExport, IO::Path::Combine(ExportDirectory, "materials"));
				break;
			case (uint32_t)RpakAssetType::Model:
				RpakFileSystem->ExportModel(AssetToExport, IO::Path::Combine(ExportDirectory, "models"), IO::Path::Combine(ExportDirectory, "animations"));
				break;
			case (uint32_t)RpakAssetType::AnimationRig:
				RpakFileSystem->ExportAnimationRig(AssetToExport, IO::Path::Combine(ExportDirectory, "animations"));
				break;
			case (uint32_t)RpakAssetType::DataTable:
				RpakFileSystem->ExportDataTable(AssetToExport, IO::Path::Combine(ExportDirectory, "datatables"));
				break;
			case (uint32_t)RpakAssetType::Subtitles:
				RpakFileSystem->ExportSubtitles(AssetToExport, IO::Path::Combine(ExportDirectory, "subtitles"));
				break;
			}

			IsCancel = StatusCallback(Asset.AssetIndex, MainForm);

			{
				std::lock_guard<std::mutex> UpdateLock(UpdateMutex);
				auto NewProgress = (uint32_t)(((float)AssetToConvert / (float)ExportAssets.Count()) * 100.f);

				if (NewProgress > CurrentProgress)
				{
					CurrentProgress = NewProgress;
					ProgressCallback(NewProgress, MainForm, false);
				}
			}
		}

		CoUninitialize();
	});

	ProgressCallback(100, MainForm, true);
}

void ExportManager::ExportVpkAssets(const std::unique_ptr<VpkLib>& VpkFileSystem, List<string>& ExportAssets)
{
	std::atomic<uint32_t> AssetIndex = 0;

	std::mutex UpdateMutex;
	uint32_t CurrentProgress = 0;
	string ExportDirectory = ExportPath;

	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "models"));
	IO::Directory::CreateDirectory(IO::Path::Combine(ExportDirectory, "animations"));

	VpkFileSystem->InitializeModelExporter((RpakModelExportFormat)Config.Get<System::SettingType::Integer>("ModelFormat"));
	VpkFileSystem->InitializeAnimExporter((RpakAnimExportFormat)Config.Get<System::SettingType::Integer>("AnimFormat"));

	Threading::ParallelTask([&VpkFileSystem, &ExportAssets, &AssetIndex, &CurrentProgress, &UpdateMutex, ExportDirectory]
	{
		(void)CoInitializeEx(0, COINIT_MULTITHREADED);

		bool IsCancel = false;

		while (AssetIndex < ExportAssets.Count() && !IsCancel)
		{
			auto AssetToConvert = AssetIndex++;

			if (AssetToConvert >= ExportAssets.Count())
				continue;

			auto& Asset = ExportAssets[AssetToConvert];
			VpkFileSystem->ExportRMdl(Asset, ExportDirectory);
		}

		CoUninitialize();
	});
}

void ExportManager::ExportRpakAssetList(std::unique_ptr<List<ApexAsset>>& AssetList, string RpakName)
{
	String ExportDirectory = IO::Path::Combine(ExportPath, "lists");
	IO::Directory::CreateDirectory(ExportDirectory);
	List<String> NameList;
	for (auto& Asset : *AssetList)
		NameList.EmplaceBack(Asset.Name);

	NameList.Sort([](const String& lhs, const String& rhs) { return lhs.Compare(rhs) < 0; });
	IO::File::WriteAllLines(IO::Path::Combine(ExportDirectory, RpakName + ".txt"), NameList);
}
