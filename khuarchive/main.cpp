#ifndef Gyeongje_archive
#define Gyeongje_archive

#include "Data.h"

/// <Archive>
PARCHIVE initialize(char **argv)
{
    FILE *fp;
    PARCHIVE archive = new ARCHIVE;
    memset(archive, 0, sizeof(ARCHIVE));

    fp = fopen(argv[0], "r+b");
    if (fp == NULL)
    {
        fp = fopen(argv[0], "w+b");
        if (fp == NULL)
            return NULL;

        archive->header.magic = 'YK'; // KY
        archive->header.version = 1;

        if (fwrite(&archive->header, sizeof(ARCHIVE_HEADER), 1, fp) < 1)
        {
            cout << "Fail write Archive Header" << endl;
            fclose(fp);
            return NULL;
        }
    }
    else
    {
        if (fread(&archive->header, sizeof(ARCHIVE_HEADER), 1, fp) < 1)
        {
            cout << "Fail read Archive Header" << endl;
            fclose(fp);
            return NULL;
        }
    }

    if (archive->header.magic != 'YK')
    {
        cout << "Not Archive File" << endl;
        fclose(fp);
        return NULL;
    }

    if (archive->header.version != 1)
    {
        cout << "Not Archive version" << endl;
        fclose(fp);
        return NULL;
    }

    archive->fp = fp;

    uint32_t size = getFileSize(fp);
    uint32_t currPos = ftell(fp);

    while (size > currPos)
    {
        PFILE_NODE node = new FILE_NODE;
        memset(node, 0, sizeof(FILE_NODE));

        if (fread(&node->desc, sizeof(FILE_DESC), 1, fp) < 1)
        {
            cout << "Fail read Archive File" << endl;
            delete node;
            finalize(archive);
            return NULL;
        }

        node->next = archive->fileList.next;
        archive->fileList.next = node;

        currPos = ftell(fp) + node->desc.size;
        fseek(fp, currPos, SEEK_SET);
    }

    return archive;
}

void finalize(PARCHIVE archive)
{
    PFILE_NODE curr = archive->fileList.next;
    while (curr != NULL)
    {
        PFILE_NODE next = curr->next;
        delete curr;

        curr = next;
    }

    fclose(archive->fp);
    delete archive;
}

bool isExist(PARCHIVE archive, string filename)
{
    PFILE_NODE curr = archive->fileList.next;
    while (curr != NULL)
    {
        if (strcmp(curr->desc.name, filename.c_str()) == 0)
            return true;

        curr = curr->next;
    }

    return false;
}

int append(PARCHIVE archive, string filename)
{
    int ret = 0;
    string buf;
    uint32_t size;

    FILE *file = fopen(filename.c_str(), "rb");
    if (file == NULL)
    {
        cout << "File open error!" << endl;
        return -1;
    }
    uint32_t file_size = getFileSize(file);
    fclose(file);

    if (file_size > 10485760) // size < 10MB
    {
        cout << "File Size < 10MB" << endl;
        return -1;
    }

    Compression(filename, buf);
    size = buf.length();

    PFILE_DESC desc = new FILE_DESC;
    memset(desc, 0, sizeof(FILE_DESC));
    strcpy(desc->name, filename.c_str());
    desc->size = size;

    PFILE_NODE node = archive->fileList.next;
    do
    {
        if (node == NULL)
        {
            fseek(archive->fp, sizeof(ARCHIVE_HEADER), SEEK_SET);
            desc->dataOffset = ftell(archive->fp) + sizeof(FILE_DESC);
        }
        else
        {
            fseek(archive->fp, node->desc.dataOffset + node->desc.size, SEEK_SET);
            desc->dataOffset = node->desc.dataOffset + node->desc.size + sizeof(FILE_DESC);
        }

        if (fwrite(desc, sizeof(FILE_DESC), 1, archive->fp) < 1)
        {
            cout << "Fail write file info" << endl;
            ret = -1;
            break;
        }

        if (fwrite(buf.c_str(), size, 1, archive->fp) < 1)
        {
            cout << "Fail write file data" << endl;
            ret = -1;
            break;
        }
        cout << filename << " Success Append File!" << endl;
        cout << "size: " << file_size << " -> " << size << endl;
    } while (0);

    delete desc;
    buf.clear();

    return ret;
}

void file_list(PARCHIVE archive)
{
    cout << "File List:" << endl;

    PFILE_NODE curr = archive->fileList.next;
    while (curr != NULL)
    {
        printf(curr->desc.name);
        curr = curr->next;
        puts("");
    }
}

int extract(PARCHIVE archive, string filename)
{
    PFILE_NODE curr = archive->fileList.next;
    uint32_t size = 0;
    uint32_t alloc_size = 0;
    uint8_t *buffer;
    string decoded;
    int ret = 0;

    while (curr != NULL)
    {
        if (strcmp(curr->desc.name, filename.c_str()) == 0)
        {
            int ret = 0;
            size = curr->desc.size;
            buffer = new uint8_t[MAX_SIZE];

            fseek(archive->fp, curr->desc.dataOffset, SEEK_SET);
            do
            {
                if (fread(buffer, size, 1, archive->fp) < 1)
                {
                    cout << "Fail read Archive File" << endl;
                    ret = -1;
                    break;
                }

                Decompression(filename, buffer, size, decoded);

                FILE *fp = fopen(filename.c_str(), "wb");
                if (fp == NULL)
                {
                    cout << filename << " Fail open file" << endl;
                    ret = -1;
                    break;
                }

                if (fwrite(decoded.c_str(), decoded.length(), 1, fp) < 1)
                {
                    cout << filename << " fail wirte file" << endl;
                    ret = -1;
                    fclose(fp);
                    break;
                }
                cout << filename << " Success Extract File" << endl;
                cout << "Size: " << size << " -> " << decoded.length() << endl;
                fclose(fp);
            } while (0);

            delete[] buffer;
            return ret;
        }

        curr = curr->next;
    }
    return -1;
}

uint32_t getFileSize(FILE *fp)
{
    uint32_t size;
    uint32_t currPos = ftell(fp);

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);

    fseek(fp, currPos, SEEK_SET);

    return size;
}

/// /Archive

/// <huffman>

string search_code(vector<code> &v, BYTE buffer)
{
    for (code item : v)
    {
        if (item.name == buffer)
            return item.huffcode;
    }
    return string("NULL");
}

bool cal_frequency(string filename, int *freq)
{
    FILE *file = fopen(filename.c_str(), "rb");

    if (file == NULL)
    {
        // cout << "File open error!" << endl;
        return ERROR;
    }

    BYTE index;

    while (fscanf(file, "%c", &index) != EOF)
    {
        ++freq[index];
    }

    fclose(file);
    return true;
}

void make_node(int *freq, priority_queue<tree, vector<tree>, Mycomp_tree> &pq)
{
    for (int i = 0; i < 256; i++)
    {
        tree item(i, freq[i]);
        pq.push(item);
    }

    return;
}

void make_tree(priority_queue<tree, vector<tree>, Mycomp_tree> &pq)
{
    while (pq.size() >= 2)
    {
        tree left = pq.top();
        pq.pop();
        tree right = pq.top();
        pq.pop();

        tree parent;
        parent.left_child.push_back(left);
        parent.right_child.push_back(right);
        parent.freq = left.freq + right.freq;

        pq.push(parent);
    }
    return;
}

bool make_code(const priority_queue<tree, vector<tree>, Mycomp_tree> &pq, priority_queue<code, vector<code>, Mycomp_code> &huffcode)
{

    inorder(pq.top(), huffcode, "");
    return true;
}

void inorder(const tree &root, priority_queue<code, vector<code>, Mycomp_code> &huffcode, string cur_code)
{

    if (!root.left_child.empty())
        inorder(root.left_child.front(), huffcode, cur_code + "0");

    if (root.left_child.empty() && root.right_child.empty())
    {
        code item;

        item.name = root.name;
        item.huffcode = cur_code;
        huffcode.push(item);
    }

    if (!root.right_child.empty())
        inorder(root.right_child.front(), huffcode, cur_code + "1");

    return;
}

bool huffman_encode(string file, priority_queue<code, vector<code>, Mycomp_code> &huffcode, string &buf)
{
    int freq[256] = {0};
    priority_queue<tree, vector<tree>, Mycomp_tree> pq;

    if (cal_frequency(file, freq) == ERROR)
        return ERROR;

    make_node(freq, pq);
    make_tree(pq);
    make_code(pq, huffcode);

    convert_binary(file, huffcode, buf);

    return true;
}

void convert_binary(string filename, priority_queue<code, vector<code>, Mycomp_code> &huffcode, string &buf)
{
    FILE *readfile = fopen(filename.c_str(), "rb");

    int dummy = 0;
    buf += dummy;
    buf += to_string((int)huffcode.size());

    int idx = 0;
    vector<code> v(huffcode.size());

    while (!huffcode.empty())
    {
        code item = huffcode.top();
        huffcode.pop();
        v[idx++] = item;

        buf += item.name;

        buf += (char)item.huffcode.length();

        BYTE buffer = 0;
        int msb = -1;

        for (unsigned int i = 0; i < item.huffcode.length(); ++i)
        {
            if (msb == 7)
            {
                buf += buffer;
                buffer = 0;
                msb = -1;
            }

            buffer = buffer << 1;
            buffer = buffer | item.huffcode[i] - '0';
            ++msb;
        }

        if (msb != -1)
        {
            while (msb != 7)
            {
                buffer = buffer << 1;
                msb++;
            }
            buf += buffer;
        }
    }

    BYTE word;
    BYTE buffer = 0;
    int msb = -1;

    while (fscanf(readfile, "%c", &word) != EOF)
    {

        string write_code = search_code(v, word);

        for (unsigned int i = 0; i < write_code.length(); ++i)
        {
            if (msb == 7)
            {
                buf += buffer;
                buffer = 0;
                msb = -1;
            }

            buffer = buffer << 1;
            buffer = buffer | write_code[i] - '0';
            ++msb;
        }
    }
    // last byte
    int lastbit = msb;
    while (lastbit != 7)
    {
        buffer = buffer << 1;
        lastbit++;
    }

    buf += buffer;
    buf[0] = msb;

    fclose(readfile);
    return;
}
// END ENCODE

bool MySort(const code &l, const code &r)
{
    return l.huffcode < r.huffcode;
}

bool decode_search_code(vector<code> &v, string &s, BYTE *word)
{
    int left = 0;
    int right = v.size() - 1;

    while (left <= right)
    {
        int mid = (left + right) / 2;

        if (v[mid].huffcode < s)
            left = mid + 1;

        else if (s < v[mid].huffcode)
            right = mid - 1;

        else
        {
            *word = v[mid].name;
            return true;
        }
    }
    return false;
}

bool huffman_decode(string name, uint8_t *buf, int size, string &decoded)
{
    vector<code> v;

    int idx = 0;
    char msb;
    char codestr[4] = {
        0,
    };
    unsigned int i, codenum = 0;

    msb = buf[idx++];
    memcpy(codestr, &buf[idx], 3);
    codenum = atoi(codestr);
    idx += 3;

    for (i = 0; i < codenum; ++i)
    {
        code item;
        char validbit;

        item.name = buf[idx++];

        validbit = buf[idx++];

        BYTE buffer = 0;
        while (validbit > 0)
        {
            buffer = buf[idx++];

            for (int j = 7; j >= 0; --j)
            {
                if (validbit <= 0)
                    break;
                char bitdata = (buffer >> j) & 1;

                item.huffcode.push_back(bitdata + '0');
                --validbit;
            }
        }
        v.push_back(item);
    }

    sort(v.begin(), v.end(), MySort);

    BYTE buffer;
    int cnt = 0;
    string huffcode;
    while (size > idx)
    {
        buffer = buf[idx++];
        if (size + 1 == idx++)
        {
            for (i = 7; i >= 7 - msb; --i)
            {
                BYTE bitdata = (buffer >> i) & 1;
                huffcode.push_back(bitdata + '0');

                BYTE write_word = 0;
                bool found = false;

                found = decode_search_code(v, huffcode, &write_word);

                if (found)
                {
                    decoded += write_word;
                    huffcode.clear();
                    break;
                }
            }
        }
        else
        {
            idx--;
            for (int i = 7; i >= 0; --i)
            {
                BYTE bitdata = (buffer >> i) & 1;
                huffcode.push_back(bitdata + '0');

                BYTE write_word = 0;
                bool found = false;

                found = decode_search_code(v, huffcode, &write_word);

                if (found)
                {
                    decoded += write_word;
                    huffcode.clear();
                }
            }
        }
    }

    return true;
}
// END DECODE

int Compression(string name, string &buf)
{
    clock_t start = clock();
    priority_queue<code, vector<code>, Mycomp_code> huffcode;

    if (huffman_encode(name, huffcode, buf) == false)
    {
        cout << "File open error!" << endl;
        return false;
    }
    cout << "Compression Time : " << ((float)clock() - start) / CLOCKS_PER_SEC << "(s)\n\n";
    return true;
}

int Decompression(string name, uint8_t *buf, int size, string &decoded)
{
    clock_t start = clock();
    if (huffman_decode(name, buf, size, decoded) == false)
    {
        cout << "File open error!" << endl;
        return false;
    }
    cout << "Decode time : " << ((float)clock() - start) / CLOCKS_PER_SEC << "(s)\n\n";
    return true;
}

void menu()
{
    cout << endl;
    cout << "     _|    _|  _|    _|  _|    _|  " << endl;
    cout << "     _|  _|    _|    _|  _|    _|  " << endl;
    cout << "     _|_|      _|_|_|_|  _|    _|  " << endl;
    cout << "     _|  _|    _|    _|  _|    _|  " << endl;
    cout << "     _|    _|  _|    _|    _|_|    " << endl;
    cout << endl;

    cout << "+------------------------------------+" << endl;
    cout << "| <ArchiveName> <Command> <Filename> |" << endl;
    cout << "+------------------------------------+" << endl;
    cout << "| Command:                           |" << endl;
    cout << "|    append:   Add File              |" << endl;
    cout << "|    list:     Print File List       |" << endl;
    cout << "|    extract:  Extract File          |" << endl;
    cout << "+------------------------------------+" << endl;
    cout << "| Example:                           |" << endl;
    cout << "|    <ArchiveName> append  <File>    |" << endl;
    cout << "|    <ArchiveName> list              |" << endl;
    cout << "|    <ArchiveName> Extract <File>    |" << endl;
    cout << "+------------------------------------+" << endl;
}

int main()
{
    menu();
    char *argv[5] = {{
        0,
    }};
    int argc = 0;
    string word;
    string input;

    cout << "Command > ";
    getline(cin, input);
    stringstream ss(input);
    while (getline(ss, word, ' '))
    {
        if (argc > 2)
            break;
        argv[argc] = new char[strlen(word.c_str()) + 1];
        strcpy(argv[argc++], word.c_str());
    }

    if (argc < 2 || argc > 3)
    {
        cout << "Plz Input <ArchiveName> <Command> <Filename>" << endl;
        for (int i = 0; i < argc; i++)
        {
            if (argv[i])
                delete[] argv[i];
        }
        return 0;
    }

    char command_[CMAX] = {
        0,
    };
    char filename_[FMAX] = {
        0,
    };
    PARCHIVE archive = initialize(argv);
    if (archive == NULL)
    {
        return 0;
    }

    if (argc == 2)
    {
        if (!strcmp("list", argv[1]))
            file_list(archive);
        else
            cout << "Not <Command>" << endl;
    }
    else if (argc > 2)
    {
        strcpy(command_, argv[1]);
        strcpy(filename_, argv[2]);
        string filename(filename_);
        string command(command_);

        if (command == "append")
        {
            if (!isExist(archive, filename))
            {
                if (append(archive, filename) == -1)
                {
                    cout << filename << " Fail Append" << endl;
                }
            }
            else
            {
                cout << "Already " << filename << endl;
            }
        }
        else if (command == "extract")
        {
            if (isExist(archive, filename))
            {
                if (extract(archive, filename) == -1)
                {
                    cout << filename << " Fail Extract" << endl;
                }
            }
            else
            {
                cout << filename << " No such file" << endl;
            }
        }
        else
        {
            cout << "Not <Command>" << endl;
        }
    }
    finalize(archive);
    for (int i = 0; i < argc; i++)
    {
        if (argv[i])
            delete[] argv[i];
    }
    return 0;
}
#endif
