#ifndef Gyeongje_Data
#define Gyeongje_Data
#define MAX_WORD 2478
#pragma warning(disable : 4996)
#define FMAX 260
#define CMAX 10
#define MAX_SIZE 10485760
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <queue>
#include <list>
#include <string>
#include <vector>
#include <ctime>
#include <cstdio>
#include <algorithm>
#include <sstream>

using namespace std;
const bool ERROR = false;
typedef unsigned char BYTE;

#define ARCHIVE_NAME "Archive.kzip"
#pragma pack(push, 1)
#pragma pack(pop)

class tree
{
public:
    tree() {}
    tree(BYTE name_, int freq_)
    {
        name = name_;
        freq = freq_;
        left_child = list<tree>();
        right_child = list<tree>();
    }
    // data

    BYTE name;
    int freq;

    // child
    list<tree> left_child, right_child;
};

class code
{
public:
    // data
    BYTE name;
    string huffcode;
};

struct Mycomp_tree
{
    bool operator()(const tree &l, const tree &r)
    {
        return l.freq > r.freq;
    }
};

struct Mycomp_code
{
    bool operator()(const code &l, const code &r)
    {
        return l.huffcode > r.huffcode;
    }
};

// Archive
typedef struct _ARCHIVE_HEADER
{
    uint16_t magic;
    uint16_t version;
} ARCHIVE_HEADER, *PARCHIVE_HEADER;

typedef struct _FILE_DESC
{
    char name[256];
    uint32_t size;
    uint32_t dataOffset;
} FILE_DESC, *PFILE_DESC;

typedef struct _FILE_NODE
{
    struct _FILE_NODE *next;
    FILE_DESC desc;
} FILE_NODE, *PFILE_NODE;

typedef struct _ARCHIVE
{
    ARCHIVE_HEADER header;
    FILE *fp;
    FILE_NODE fileList;
} ARCHIVE, *PARCHIVE;

/// <ARCHIVE>
PARCHIVE initialize();
void finalize(PARCHIVE);
bool isExist(PARCHIVE, string);
int append(PARCHIVE, string);
void file_list(PARCHIVE);
int extract(PARCHIVE, string);
uint32_t getFileSize(FILE *);
/// </ARCHIVE>

/// <ENCODE>
string search_code(vector<code> &, BYTE);
bool cal_frequency(string, int *);
void make_node(int *, priority_queue<tree, vector<tree>, Mycomp_tree> &);
void make_tree(priority_queue<tree, vector<tree>, Mycomp_tree> &);
void inorder(const tree &, priority_queue<code, vector<code>, Mycomp_code> &, string);
bool make_code(const priority_queue<tree, vector<tree>, Mycomp_tree> &, priority_queue<code, vector<code>, Mycomp_code> &);
bool huffman_encode(string, priority_queue<code, vector<code>, Mycomp_code> &, string &);
void convert_binary(string, priority_queue<code, vector<code>, Mycomp_code> &, string &);
/// </ENCODE>

/// <DECODE>
bool huffman_decode(string, uint8_t *, string &);
bool decode_search_code(vector<code> &, string &, BYTE *);
bool MySort(const code &, const code &);
/// </DECODE>

int Compression(string, string &);
int Decompression(string, uint8_t *, int, string &);
void menu();

#endif
