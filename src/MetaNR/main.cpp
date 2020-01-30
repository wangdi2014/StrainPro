#include "structure.h"
#include <sys/stat.h>
#include "../version.h"

time_t StartProcessTime;

bwt_t *Refbwt;
int iThreadNum;
bwaidx_t *RefIdx;
int64_t RefSeqSize, DoubleRefSeqSize;
char *RefSequence, *IndexPrefix, *OutputFASTA;
string NodesDumpFilePath = "taxonomy/nodes.dmp";
string MergedDumpFilePath = "taxonomy/merged.dmp";

void ShowProgramUsage(const char* program)
{
	fprintf(stderr, "\n");
	fprintf(stderr, "%s v%s\n", program, VERSION);
	fprintf(stderr, "Usage: %s -i IndexPrefix\n\n", program);
	fprintf(stderr, "Options: -t     INT     number of threads [%d]\n", iThreadNum);
	fprintf(stderr, "         -o     STR     output fasta [%s]\n", OutputFASTA);
	fprintf(stderr, "         -dump  STRING  dump file path\n");
	fprintf(stderr, "\n");
}

void LoadDumpFilePath(const char* filename)
{
	fstream file, f;
	stringstream ss;
	string str, s1, s2;

	file.open(filename, ios_base::in);
	if (!file.is_open())
	{
		fprintf(stderr, "cannot open file %s\n", filename);
		exit(1);
	}
	while (!file.eof())
	{
		getline(file, str); if (str == "") continue;
		ss.clear(); ss.str(str); ss >> s1 >> s2;
		if (s1 == "NodesDumpFilePath")
		{
			f.close(); f.open(s2.c_str());
			if (!f.is_open())
			{
				fprintf(stderr, "Error! File (%s) is not accessible\n", s2.c_str());
				exit(1);
			}
			else NodesDumpFilePath = s2;
		}
		else if (s1 == "MergedDumpFilePath")
		{
			f.close(); f.open(s2.c_str());
			if (!f.is_open())
			{
				fprintf(stderr, "Error! File (%s) is not accessible\n", s2.c_str());
				exit(1);
			}
			else MergedDumpFilePath = s2;
		}
	}
	file.close();

	fprintf(stderr, "Use the dump files: %s and %s\n", NodesDumpFilePath.c_str(), MergedDumpFilePath.c_str());
}

bool CheckBWAIndexFiles()
{
	char fn[1024];
	bool bRet = true;
	struct stat buffer;

	sprintf(fn, "%s.ann", IndexPrefix); if (stat(fn, &buffer) != 0) bRet = false, fprintf(stderr, "Cannot access file: %s\n", fn);
	sprintf(fn, "%s.amb", IndexPrefix); if (stat(fn, &buffer) != 0) bRet = false, fprintf(stderr, "Cannot access file: %s\n", fn);
	sprintf(fn, "%s.pac", IndexPrefix); if (stat(fn, &buffer) != 0) bRet = false, fprintf(stderr, "Cannot access file: %s\n", fn);
	sprintf(fn, "%s.bwt", IndexPrefix); if (stat(fn, &buffer) != 0) bRet = false, fprintf(stderr, "Cannot access file: %s\n", fn);
	sprintf(fn, "%s.sa",  IndexPrefix); if (stat(fn, &buffer) != 0) bRet = false, fprintf(stderr, "Cannot access file: %s\n", fn);

	return bRet;
}

int main(int argc, char* argv[])
{
	int i;
	string parameter, str;

	iThreadNum = 16;
	OutputFASTA = (char*)"output.fasta";

	if (argc == 1 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
	{
		ShowProgramUsage(argv[0]);
		exit(0);
	}
	else
	{
		for (i = 1; i < argc; i++)
		{
			parameter = argv[i];

			if (parameter == "-i" && i + 1 < argc) IndexPrefix = argv[++i];
			else if (parameter == "-o" && i + 1 < argc) OutputFASTA = argv[++i];
			else if (parameter == "-t" && i + 1 < argc)
			{
				if ((iThreadNum = atoi(argv[++i])) < 0) iThreadNum = 16;
			}
			else if (parameter == "-dump" && ++i < argc) LoadDumpFilePath(argv[i]);
			else fprintf(stderr, "Warning! Unknow parameter: %s\n", argv[i]);
		}
	}
	StartProcessTime = time(NULL);

	if (IndexPrefix == NULL || CheckBWAIndexFiles() == false)
	{
		fprintf(stderr, "\n\nError! Index files are corrupt!\n");
		exit(1);
	}
	GetTaxInfomation();
	RefIdx = bwa_idx_load(IndexPrefix);
	if (RefIdx == 0)
	{
		fprintf(stderr, "\n\nError! Index files are corrupt!\n");
		exit(1);
	}
	else
	{
		Refbwt = RefIdx->bwt;
		RestoreReferenceInfo();
	}
	SeqTaxing();
	fprintf(stderr, "Done! It took %lld seconds.\n\n", (long long)(time(NULL) - StartProcessTime));
	
	bwa_idx_destroy(RefIdx);

	return 0;
}
