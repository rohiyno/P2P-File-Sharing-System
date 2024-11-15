#include "headers.h"

long long file_size(char *path)
{
    FILE *fp = fopen(path, "rb");

    long size = -1;
    if (fp)
    {
        fseek(fp, 0, SEEK_END);
        size = ftell(fp) + 1;
        fclose(fp);
    }
    else
    {
        printf("File not found.\n");
        return -1;
    }
    return size;
}

void getStringHash(string segmentString, vector<string> &chunksHash)
{
    unsigned char md[20];
    if (SHA1(reinterpret_cast<const unsigned char *>(&segmentString[0]), segmentString.length(), md))
    {
        std::stringstream ss;
        ss << std::hex << std::setw(2) << std::setfill('0');
        for (int i = 0; i < 20; ++i)
        {
            ss << static_cast<int>(md[i]);
        }
        string hash = ss.str();
        chunksHash.push_back(hash);
    }
    else
    {
        std::cerr << "Error in hashing\n";
    }
}

/*************************************************************/
/*        Returns combined PIECEWISE hash of the file        */
/*************************************************************/
vector<string> getHash(char *path)
{

    int i, accum;
    FILE *fp1;
    vector<string> chunksHash;
    long long fileSize = file_size(path);
    if (fileSize == -1)
    {
        return chunksHash;
    }
    int segments = fileSize / 524288 + 1;
    char line[32768 + 1];

    fp1 = fopen(path, "r");

    if (fp1)
    {
        for (i = 0; i < segments; i++)
        {
            accum = 0;
            string segmentString;

            int rc;
            while (accum < 524288 && (rc = fread(line, 1, min(32768 - 1, 524288 - accum), fp1)))
            {
                line[rc] = '\0';
                accum += strlen(line);
                segmentString += line;
                memset(line, 0, sizeof(line));
            }

            getStringHash(segmentString, chunksHash);
        }

        fclose(fp1);
    }
    else
    {
        printf("File not found.\n");
    }

    return chunksHash;
}

string getFileHash(char *path)
{
    std::ostringstream buf;
    std::ifstream input(path);
    buf << input.rdbuf();
    std::string contents = buf.str();
    std::string hash;

    unsigned char md[SHA256_DIGEST_LENGTH];
    if (SHA256(reinterpret_cast<const unsigned char *>(contents.c_str()), contents.length(), md))
    {
        std::stringstream ss;
        ss << std::hex << std::setw(2) << std::setfill('0');
        for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        {
            ss << static_cast<int>(md[i]);
        }
        hash = ss.str();
    }
    else
    {
        std::cerr << "Error in hashing\n";
    }

    return hash;
}

unordered_map<long long, string> calculateSHAOfAllChunks(string &filename)
{
    unordered_map<long long, string> shaMap;

    FILE *file = fopen(filename.c_str(), "rb");
    if (!file)
    {
        std::cerr << "Error: Could not open file." << std::endl;
        return shaMap;
    }
    int chunkSize = 524288;
    char *buffer = new char[chunkSize];
    long long chunkNumber = 0;
    size_t bytesRead = 0;

    while ((bytesRead = fread(buffer, 1, chunkSize, file)) > 0)
    {
        SHA_CTX shaContext;
        SHA1_Init(&shaContext);
        SHA1_Update(&shaContext, buffer, bytesRead);
        unsigned char shaResult[SHA_DIGEST_LENGTH];
        SHA1_Final(shaResult, &shaContext);

        std::stringstream shaStream;
        shaStream << std::hex << std::setfill('0');
        for (int i = 0; i < SHA_DIGEST_LENGTH; ++i)
        {
            shaStream << std::setw(2) << static_cast<unsigned>(shaResult[i]);
        }

        shaMap[chunkNumber++] = shaStream.str();
    }

    fclose(file);
    return shaMap;
}

string getChunkHash(char *buffer, long bytesRead)
{
    SHA_CTX shaContext;
    SHA1_Init(&shaContext);
    SHA1_Update(&shaContext, buffer, bytesRead);
    unsigned char shaResult[SHA_DIGEST_LENGTH];
    SHA1_Final(shaResult, &shaContext);

    std::stringstream shaStream;
    shaStream << std::hex << std::setfill('0');
    for (int i = 0; i < SHA_DIGEST_LENGTH; ++i)
    {
        shaStream << std::setw(2) << static_cast<unsigned>(shaResult[i]);
    }
    return shaStream.str();
}