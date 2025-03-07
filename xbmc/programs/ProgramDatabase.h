#pragma once

#include "dbwrappers/Database.h"

#ifdef _LINUX
#include "PlatformDefs.h" // FILETIME
#endif

typedef std::vector<CStdString> VECPROGRAMPATHS;

#define COMPARE_PERCENTAGE     0.90f // 90%
#define COMPARE_PERCENTAGE_MIN 0.50f // 50%

class CFileItem;

class CProgramDatabase : public CDatabase
{
public:
  CProgramDatabase(void);
  virtual ~CProgramDatabase(void);
  virtual bool Open();

  bool AddTrainer(int iTitleId, const CStdString& strText);
  bool RemoveTrainer(const CStdString& strText);
  bool GetTrainers(unsigned int iTitleId, std::vector<CStdString>& vecTrainers);
  bool GetAllTrainers(std::vector<CStdString>& vecTrainers);
  bool SetTrainerOptions(const CStdString& strTrainerPath, unsigned int iTitleId, unsigned char* data, int numOptions);
  bool GetTrainerOptions(const CStdString& strTrainerPath, unsigned int iTitleId, unsigned char* data, int numOptions);
  void SetTrainerActive(const CStdString& strTrainerPath, unsigned int iTitleId, bool bActive);
  CStdString GetActiveTrainer(unsigned int iTitleId);
  bool HasTrainer(const CStdString& strTrainerPath);
  bool ItemHasTrainer(unsigned int iTitleId);

  int GetRegion(const CStdString& strFilenameAndPath);
  bool SetRegion(const CStdString& strFilenameAndPath, int iRegion=-1);

  uint32_t GetTitleId(const CStdString& strFilenameAndPath);
  bool SetTitleId(const CStdString& strFilenameAndPath, uint32_t dwTitleId);
  bool IncTimesPlayed(const CStdString& strFileName1);
  bool SetDescription(const CStdString& strFileName1, const CStdString& strDescription);
  bool GetXBEPathByTitleId(const uint32_t titleId, CStdString& strPathAndFilename);

  uint32_t GetProgramInfo(CFileItem *item);
  bool AddProgramInfo(CFileItem *item, unsigned int titleID);

protected:
  virtual bool CreateTables();
  virtual bool UpdateOldVersion(int version);
  virtual int GetMinVersion() const { return 3; };
  const char *GetBaseDBName() const { return "MyPrograms"; };

  FILETIME TimeStampToLocalTime( uint64_t timeStamp );
};
