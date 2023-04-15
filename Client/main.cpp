//
//  client.cc
//
//  Copyright (c) 2019 Yuji Hirose. All rights reserved.
//  MIT License
//

#include "../common/httplib/httplib.hpp"
#include "../common/b64/b64.hpp"
#include "../common/SHA256/sha256.h"
#include <iostream>
#include <sys/stat.h>
#include <sstream>
#define RESET "\033[0m"
#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"              /* Green */
#define YELLOW "\033[33m"             /* Yellow */
#define BLUE "\033[34m"               /* Blue */
#define MAGENTA "\033[35m"            /* Magenta */
#define CYAN "\033[36m"               /* Cyan */
#define WHITE "\033[37m"              /* White */
#define BOLDBLACK "\033[1m\033[30m"   /* Bold Black */
#define BOLDRED "\033[1m\033[31m"     /* Bold Red */
#define BOLDGREEN "\033[1m\033[32m"   /* Bold Green */
#define BOLDYELLOW "\033[1m\033[33m"  /* Bold Yellow */
#define BOLDBLUE "\033[1m\033[34m"    /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m" /* Bold Magenta */
#define BOLDCYAN "\033[1m\033[36m"    /* Bold Cyan */
#define BOLDWHITE "\033[1m\033[37m"   /* Bold White */
#define RESET "\033[0m"
#define BLACK "\033[30m"   /* Black */
#define RED "\033[31m"     /* Red */
#define GREEN "\033[32m"   /* Green */
#define YELLOW "\033[33m"  /* Yellow */
#define BLUE "\033[34m"    /* Blue */
#define MAGENTA "\033[35m" /* Magenta */
#define CYAN "\033[36m"    /* Cyan */
#define CA_CERT_FILE "./ca-bundle.crt"
const int BLOCKSIZE = 1024 * 0.75;
char Block[BLOCKSIZE]{0};

using namespace std;
SHA256 hasher;
long GetFileSize(std::string filename)
{
    struct stat64 stat_buf;
    int rc = stat64(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}
long FdGetFileSize(int fd)
{
    struct stat64 stat_buf;
    int rc = fstat64(fd, &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}
bool exist(httplib::Client &cli, string filename)
{
    auto res = cli.Get("/exists/" + filename);
    if (res)
    {
        if (strcmp(res->body.c_str(), "0") == 0)
            return 0;
    }
    else
        return NULL;
    return 1;
}
map<string, int> sent;
void DDWrite(httplib::Client &cli, string filename)
{
    long filesize = GetFileSize(filename);

    ifstream F(filename);
    ofstream hashs(filename + ".hashs");
    string C;
    string Hash;
    int blocks = 0;
    int equalcount = 0;
    cout << endl;
    while (!F.eof())
    {
        blocks++;
        // cout<<endl;
        cout << BOLDGREEN << "\r[ " << RESET; //<<blocks<<"/"<<filesize/BLOCKSIZE;
        equalcount = 0;
        for (double i = 0; i <= ((double)blocks * BLOCKSIZE / (double)filesize); i += 0.019)
        {
            cout << GREEN << "=";
            equalcount++;
        }
        for (int i = equalcount; i < 50; i++)
        {
            cout << YELLOW << "=";
        }

        cout << BOLDGREEN << " ] " << 100 * ((double)blocks * BLOCKSIZE / (double)filesize) << RESET;

        // std::streamsize s = ((F) ? BLOCKSIZE : F.gcount());
        hasher.init();
        F.read(Block, BLOCKSIZE);
        C = Block;
        // cout<<"\n/append/" + filename + "/" + sha256(C)<<endl;
        // sleep(2);
        cli.Get("/append/" + b64encode(filename) + "/" + sha256(C) + ",");
        if (!sent[sha256(C)])
            if (!exist(cli, sha256(C)))

                cli.Get("/write/" + sha256(C) + "/" + b64encode(C)), sent[sha256(C)] = 1;
            else
                sent[sha256(C)] = 1;
        else
            sent[sha256(C)]++;

        hashs << sha256(C) << ",";
        for (int i = 0; i < BLOCKSIZE; i++)
            Block[i] = 0;
    }
    F.close();
    hashs.close();
    cout << "done" << endl;
}
string removebranch(string pathfile)
{
    return pathfile.substr(pathfile.find_last_of("/\\") + 1);
}
vector<string> hashs;
void DDRead_Step2(httplib::Client &cli, ofstream &file)
{
    for (int i = 0; i < hashs.size(); i++)
    {
        auto res = cli.Get("/read/" + hashs[i]);
        if (res)
        {
            // cout << sha256(b64decode(res->body)) << " <=>" << hashs[i] << endl;
            file << b64decode(res->body).c_str();
        }
        else
        {
            cout << "error" << endl;
        }
    }
}
void gethashs_step2(string hashssepcomma)
{
    hashs.clear();
    string buffer;
    for (int i = 0; i < hashssepcomma.size(); i++)
    {
        if (hashssepcomma[i] == ',')
            hashs.push_back(buffer), buffer = "";
        else
            buffer.push_back(hashssepcomma[i]);
        // cout<<hashssepcomma[i]<<endl;
    }
}
void gethashs(string hashfile)
{

    ifstream hashsfile(hashfile);
    stringstream ss;
    ss << hashsfile.rdbuf();
    string hashssepcomma = ss.str();
    gethashs_step2(hashssepcomma);
    hashsfile.close();
    // return hashs;
}
void DDRead(httplib::Client &cli, vector<string> &hashs, string filename)
{
    ofstream file(removebranch(filename));
    // cout << removebranch(filename) << endl;
    DDRead_Step2(cli, file);
    file.close();
}
void DDRead(httplib::Client &cli, string filename)
{
    cout << removebranch(filename) << endl;
    ofstream file(removebranch(filename));
    auto res = cli.Get("/download/" + b64encode(filename));
    if (res)
    {
        cout<<res->body<<endl;
        gethashs_step2(res->body);
    }
    else
    {
        cout << "error" << endl;
    }
    DDRead_Step2(cli, file);

    file.close();
}
void DDlist(httplib::Client &cli)
{
    auto res = cli.Get("/list");
    if (res)
        gethashs_step2(res->body);
    // cout<<res->body<<endl;
}
class Commandlineinterface
{
public:
    Commandlineinterface()
    {
    }
    void Ls(httplib::Client &cli)
    {
        DDlist(cli);
        for (int i = 0; i < hashs.size(); i++)
        {
            cout << b64decode(hashs[i]) << endl;
        }
    }
    void Upload(httplib::Client &cli, string filename)
    {
        DDWrite(cli, filename);
    }
    void Download(httplib::Client &cli, string filename)
    {
        DDRead(cli, filename);
    }
    void Remove(httplib::Client &cli, string filename)
    {
        auto res = cli.Get("/remove/" + b64encode(filename));
    }
    void Size(httplib::Client &cli, string filename)
    {
        auto res = cli.Get("/download/" + b64encode(filename));
        // cout<<res->body<<endl;
        gethashs_step2(res->body);
        cout << hashs.size() << "blocks of " << BLOCKSIZE << " char long" << endl;
    }
};
string replacecharparchar(string s, char in, char out)
{
    string buffer = "";
    for (int i = 0; i < s.size(); i++)
        if (s[i] == out)
            buffer.push_back(in);
        else
            buffer.push_back(s[i]);
    return buffer;
}
int main(void)
{
    hasher = SHA256();
    httplib::Client client("localhost", 8080);
    Commandlineinterface cli;
    string buff = "";
    char* Buff;
    cout << "usage: Upload,Download,Remove\",\"filename Ls exist but dosnt need any param" << endl;
    while (true)
    {
        cout << "\r->";
        //cin >> buff;
        cin.getline(Buff,1024);
        buff=Buff;
        cout<<replacecharparchar(buff, ',', ' ')<<endl;
        gethashs_step2(replacecharparchar(buff, ',', ' ') + ",,");

        for (int i = 0; i < hashs.size(); i++)
        {

            if (strcmp(hashs[i].c_str(), "Upload") == 0)
            {
                cli.Upload(client, hashs[i + 1]);
            }
            if (strcmp(hashs[i].c_str(), "Download") == 0)
            {
                cli.Download(client, hashs[i + 1]);
            }
            if (strcmp(hashs[i].c_str(), "Ls") == 0)
            {
                cli.Ls(client);
            }
            if (strcmp(hashs[i].c_str(), "Size") == 0)
            {
                cli.Size(client, hashs[i + 1]);
            }
            if (strcmp(hashs[i].c_str(), "Remove") == 0)
            {
                cli.Remove(client, hashs[i + 1]);
            }
        }
    }

    return 0;
}