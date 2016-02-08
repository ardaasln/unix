// Arinc Elhan
// e1819291

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>

// check whether the file has ".digs" extension or not
int checkfilename(const char* filename)
{
	int i = 0, j = 0, k;
	char c = filename[i];
	while (c != 0)
	{
		i++;j++;
		c = filename[i];
	}
	char* subbuf = (char*) malloc(sizeof(char) * 5);
	for (k = 0; k < 5; k++)
		subbuf[k] = filename[j + k - 5];
	subbuf[5] = 0;
	if (strcmp(subbuf, ".digs") == 0)
		return 1;
	else
		return 0;
}

// update or create dig file function
void check(const char* filename, int size)
{
	int i, n, m;
	FILE *file, *digFile;

	char readbuffer[128 * 1024];
	unsigned char digest[128];
	unsigned char checkdigest[128];

	unsigned char dig_name[256];
	snprintf(dig_name, sizeof(dig_name), "%s.digs", filename);

	digFile	= fopen(dig_name, "w");
	file = fopen(filename, "r");

	int loopNum = size / (1024 * 128);
	int remain	= size % (1024 * 128);
	
	for (i = 0; i < loopNum; i++)
	{
		fread(readbuffer, 1, 1024 * 128, file);
		n = digmd5(readbuffer, digest, 128 * 1024);
		fwrite(digest, 1, n, digFile);
	}

	if (remain != 0)
	{
		fread(readbuffer, 1, remain, file);
		n = digmd5(readbuffer, digest, remain);
		fwrite(digest, 1, n, digFile);
	}
	fclose(file);
	fclose(digFile);
}

// compare source and destination files and update them 
void compare(const char* file1, const char* file2)
{
	int i,n,m;
	FILE *sourceFile, *destFile, *sourceDigest, *destDigest;

	char sourceBuffer[128 * 1024], destBuffer[128 * 1024];
	unsigned char source_digest[128], dest_digest[128];
	unsigned char source_dig_name[256], dest_dig_name[256];

	struct stat sourceStatBuffer, destStatBuffer;
	struct stat sourceDigBuffer, destDigBuffer;

	snprintf(source_dig_name, sizeof(source_dig_name), "%s.digs", file1);
	snprintf(dest_dig_name, sizeof(dest_dig_name), "%s.digs", file2);

	sourceFile	= fopen(file1, "r+");
	destFile	= fopen(file2, "r+");
	sourceDigest= fopen(source_dig_name, "r+");
	destDigest	= fopen(dest_dig_name, "r+");

	stat(file1, &sourceStatBuffer);
	stat(file2, &destStatBuffer);
	stat(source_dig_name, &sourceDigBuffer);
	stat(dest_dig_name, &destDigBuffer);

	int sourceSize = sourceDigBuffer.st_size;
	int destSize   = destDigBuffer.st_size;

	int source_loopnum	= sourceSize / 16;
	int dest_loopnum	= destSize / 16;

	if (source_loopnum >= dest_loopnum)
	{
		for (i = 0; i < dest_loopnum; i++)
		{
			fread(source_digest, 1, 16, sourceDigest);
			fread(dest_digest, 1, 16, destDigest);

			n = fread(sourceBuffer, 1, 1024 * 128, sourceFile);
			m = fread(destBuffer, 1, 1024 * 128, destFile);

			if (strcmp(source_digest, dest_digest) != 0)
			{
				fseek(destFile, -m, SEEK_CUR);
				fwrite(sourceBuffer, 1, n, destFile);
			}
		}
		for (i = dest_loopnum; i < source_loopnum; i++)
		{
			n = fread(sourceBuffer, 1, 1024 * 128, sourceFile);
			fwrite(sourceBuffer, 1, n, destFile);
		}
	}
	else
	{
		for(i = 0; i < source_loopnum; i++)
		{
			fread(source_digest, 1, 16, sourceDigest);
			fread(dest_digest, 1, 16, destDigest);

			n = fread(sourceBuffer, 1, 1024 * 128, sourceFile);
			m = fread(destBuffer, 1, 1024 * 128, destFile);

			if (strcmp(source_digest, dest_digest) != 0)
			{
				fseek(destFile, -m, SEEK_CUR);
				fwrite(sourceBuffer, 1, n, destFile);
			}
		}
	}
	fclose(sourceFile);
	fclose(destFile);
	fclose(sourceDigest);
	fclose(destDigest);
}

// digestion file creation function
void digFile(const char* file)
{
	unsigned char dig[256];
	snprintf(dig, sizeof(dig), "%s.dig", file);
	struct stat buffer;
	stat(file, &buffer);
	int size = buffer.st_size;

	check(file, size);
}

// if destination file does not exist, use this function
void destNotExist(const char* file1, const char* file2)
{
	FILE* sourceFile = fopen(file1, "r");
	FILE* destFile	 = fopen(file2, "w");
	struct stat buffer;
	stat(file1, &buffer);
	int size = buffer.st_size;
	digFile(file1);

	unsigned char *fileBuffer = malloc(sizeof(char) * size);
	fread(fileBuffer, 1, size, sourceFile);
	fwrite(fileBuffer, 1, size, destFile);

	fclose(sourceFile);
	fclose(destFile);

	digFile(file2);
}

// if destination file exists, use this function
void destExist(const char* file1, const char* file2)
{
	FILE* sourceFile = fopen(file1, "r");
	FILE* destFile	 = fopen(file2, "r+");
	unsigned char destDig[256];
	snprintf(destDig, sizeof(destDig), "%s.digs",file2);
	FILE* dDigFile = fopen(destDig, "r");
	
	struct stat destBuffer, digBuffer;
	stat(destDig, &digBuffer);
	stat(file2, &destBuffer);
	if ( dDigFile == NULL || destBuffer.st_mtime > digBuffer.st_mtime)
		digFile(file2);
	digFile(file1);
	compare(file1, file2);
	digFile(file2);
}

// this function is used for copying directories
void listdir(const char* filename, const char* destname)
{
	struct dirent *pDirent;
	struct stat st;
	DIR *pDir;
    pDir = opendir(filename);

	char destfile[1024];
	char sourcefile[1024];
	
	if (pDir == NULL)
	{
		perror("Source error");
        return;
    }
	
    while ((pDirent = readdir(pDir)) != NULL)
	{
		int len = snprintf (destfile, sizeof(destfile)-1, "%s/%s/%s", destname, filename, pDirent->d_name);
		int len2 = snprintf(sourcefile, sizeof(sourcefile)-1, "%s/%s", filename, pDirent->d_name);
		destfile[len] = 0;
		sourcefile[len2] = 0;
		//printf("%s\n%s\n", destfile, sourcefile);	
		if (pDirent->d_type == DT_REG)
		{
			continue;
		}
		else
		{
			if (strcmp(pDirent->d_name, ".") == 0 || strcmp(pDirent->d_name, "..") == 0)
				continue;
			else
			{
				if (stat(destfile, &st) == -1)
				{
					mkdir(destfile, 0700);
				}
				listdir(sourcefile, destname);
			}
		}
    }
    closedir (pDir);
}

// this function is used for copying files
void listdir2(const char* filename, const char* destname)
{
	struct dirent *pDirent;
	struct stat st;
	DIR *pDir;
    pDir = opendir(filename);

	char destfile[1024];
	char sourcefile[1024];

	if (pDir == NULL)
	{
		perror("Source error");
        return;
    }
	
    while ((pDirent = readdir(pDir)) != NULL)
	{
		int len = snprintf (destfile, sizeof(destfile)-1, "%s/%s/%s", destname, filename, pDirent->d_name);
		int len2 = snprintf(sourcefile, sizeof(sourcefile)-1, "%s/%s", filename, pDirent->d_name);
		destfile[len] = 0;
		sourcefile[len2] = 0;
		
		if (pDirent->d_type == DT_REG)
		{

			if (checkfilename(sourcefile) == 1)
			{
				continue;
			}

			FILE* dFile = fopen(destfile, "r");
			if (dFile == NULL)
				destNotExist(sourcefile, destfile);
			else
				destExist(sourcefile, destfile);
		}
		else
		{
			if (strcmp(pDirent->d_name, ".") == 0 || strcmp(pDirent->d_name, "..") == 0)
				continue;
			else
			{
				listdir2(sourcefile, destname);
			}
		}
    }
    closedir (pDir);
}

int main(int argc, char *argv[])
{
	if (argc < 3)
	{
		printf("Correct usage: lcopy [-r] source dest\n");
		return 0;
	}
	if (argc == 3 && (strcmp(argv[1], "-r") == 0 || strcmp(argv[2], "-r") == 0))
	{
		printf("Correct usage: lcopy [-r] source dest\n");
		return 0;
	}
	
	int i, j;
	int destType = -1;
	int destStat;
	struct stat destBuffer;

	destStat = stat(argv[argc-1], &destBuffer);
	
	// if destination is a regular file
	if (S_ISREG(destBuffer.st_mode))
		destType = 0;

	// if destination is a directory
	else if (S_ISDIR(destBuffer.st_mode))
		destType = 1;

	// destination is a regular file
	if (destType == 0 || destType == -1)
	{
		if (argc > 3 || strcmp(argv[1], "-r") == 0)
		{
			printf("Destination does not exist or is a file.\n");
			return 0;
		}
		else
		{
			int sourceStat;
			struct stat sourceBuffer;

			sourceStat = stat(argv[1], &sourceBuffer);
			if (sourceStat != 0)
			{
				perror("Source Error");
				return 0;
			}
			if(S_ISDIR(sourceBuffer.st_mode))
			{
				printf("Destination is a file, directories cannot be copied.\n");
				return 0;
			}
			if (checkfilename(argv[1]) == 1)
			{
				printf("%s file is 'dig'. Please choose another file.\n", argv[1]);
				return;
			}
			if (destType == -1)
				destNotExist(argv[1], argv[2]);
			else
				destExist(argv[1], argv[2]);
		}
	}

	// if destination is a directory
	else if (S_ISDIR(destBuffer.st_mode))
	{
		char currentpath[1024];
		getcwd(currentpath, 1024);
		
		int recornot = -1;

		DIR *pDir;
		struct dirent *pDirent;
		pDir = opendir(argv[argc-1]);
				
		if (pDir == NULL)
		{
			printf("Cannot open directory.\n");
			return 0;
		}

		if (strcmp(argv[1], "-r") == 0)
		{
			recornot = 1;
			i = 2;
		}
		else
		{
			recornot = 0;
			i = 1;
		}
		for (; i < argc - 1; i++)
		{
			if (checkfilename(argv[i]) == 1)
			{
				printf("%s is 'dig'. Please choose another file.\n", argv[i]);
				continue;
			}
			chdir(currentpath);

			int sourceStat;
			struct stat sourceBuffer;
			sourceStat = stat(argv[i], &sourceBuffer);

			if (sourceStat != 0)
			{
				perror("Source error");
				continue;
			}
			else if (S_ISDIR(sourceBuffer.st_mode))
			{
				if (recornot == 0)
				{
					printf("%s is a directory. Use [-r] for copying it.\n", argv[i]);
					continue;
				}
				else
				{
					chdir(argv[argc-1]);
					char filepath[1024];
					getcwd(filepath, 1024);
					chdir(currentpath);

					chdir(argv[i]);
					char filepath2[1024];
					getcwd(filepath2, 1024);
					chdir(currentpath);

					char filewrite[1024];
					char* relative;
					relative = strrchr(filepath2, '/')+1;
					int len1 = snprintf(filewrite, sizeof(filewrite) - 1, "%s/%s", filepath, relative);
					filewrite[len1] = 0;
					struct stat st;
					if (stat (filewrite, &st) == -1)
					{
						mkdir(filewrite, 0700);
					}
					listdir(relative, argv[argc-1]);
					listdir2(relative, argv[argc-1]);
				}
			}
			else if (S_ISREG(sourceBuffer.st_mode))
			{
				chdir(argv[argc-1]);
				char filepath[1024];
				getcwd(filepath, 1024);
				chdir(currentpath);

				char filewrite[1024];

				char* relative;
				relative = strrchr(argv[i], '/');
				int len1 = snprintf(filewrite, sizeof(filewrite) - 1, "%s%s", filepath, relative);
				filewrite[len1] = 0;
				if (relative == NULL)
				{
					len1 = snprintf(filewrite, sizeof(filewrite) - 1, "%s/%s", filepath, argv[i]);
					filewrite[len1] = 0;
				}
				FILE* destFile = fopen(filewrite, "r");
				if (destFile == NULL)
					destNotExist(argv[i], filewrite);
				else
					destExist(argv[i], filewrite);
			}
		}
	}
	return 0;
}
