#pragma once

typedef struct SFileEntry
{
	std::string PATH;
	std::string MD5;
} FileEntry;

class ShieldHelper : public CSingleton <ShieldHelper>
{
public:
	ShieldHelper();
	~ShieldHelper();

	bool ActivateProtection();
	void ProtectReadProcessMemory();
	void AddFileEntry(std::string path, std::string hash);

	bool CheckFiles();

protected:
	std::vector<FileEntry> Checklist;

};

