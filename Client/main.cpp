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
const int BLOCKSIZE=256;
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
bool DDWrite(httplib::Client &cli, string filename)
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
        cout << BOLDGREEN << "\r[ " << RESET;
        equalcount = 0;
        for (double i = 0; i <= ((double)blocks * BLOCKSIZE / (double)filesize); i += 0.035)
        {
            cout << GREEN << "=";
            equalcount++;
        }
        for (int i = equalcount; i < 29; i++)
        {
            cout << YELLOW << "=";
        }
        cout << BOLDGREEN << " ] " << RESET << blocks / BLOCKSIZE << " Mb";

        // std::streamsize s = ((F) ? BLOCKSIZE : F.gcount());
        hasher.init();
        F.read(Block, BLOCKSIZE);
        C = Block;
        if (!sent[sha256(C)])
        {
            if (!exist(cli, sha256(C)))
            {
                cli.Get("/write/" + /*SHA1::from_file(".temp")*/ sha256(C) + "/" + b64encode(C));
                sent[sha256(C)] = 1;
                // cout << "sent" << sha256(C) << endl;
            }
            else
            {
                // cout << "skipped block:" << sha256(C) << " (Already exist)" << endl;
                sent[sha256(C)] = 1;
            }
        }
        else
        {
            sent[sha256(C)]++;
            // cout << "skipped block:" << sha256(C) << " (Already sent)"<<sent[sha256(C)] << endl;
        }
        hashs << sha256(C) << ",";
        for (int i = 0; i < BLOCKSIZE; i++)
            Block[i] = 0;
    }
    F.close();
    hashs.close();
    cout << "done" << endl;
}
bool DDRead(httplib::Client &cli, vector<string> &hashs,string filename)
{
    ofstream file(filename);
    for (int i = 0; i < hashs.size(); i++)
    {
            auto res=cli.Get("/read/"+hashs[i]);
            file<<b64decode(res->body).c_str();
            cout<<sha256(b64decode(res->body))<<endl;
    }
}
vector<string> hashs;
void gethashs(string hashfile){
    hashs.clear();
    ifstream hashsfile(hashfile);
    stringstream ss;
    ss<<hashsfile.rdbuf();
    string hashssepcomma=ss.str();
    string buffer;
    for(int i=0;i<hashssepcomma.size();i++){
        if(hashssepcomma[i]==',')
        hashs.push_back(buffer),buffer="";
        else buffer.push_back(hashssepcomma[i]);
    }
    //return hashs;

}
int main(void)
{
    hasher = SHA256();
    /*
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
  httplib::SSLClient cli("localhost", 8080);
  // httplib::SSLClient cli("google.com");
  // httplib::SSLClient cli("www.youtube.com");
  cli.set_ca_cert_path(CA_CERT_FILE);
  cli.enable_server_certificate_verification(true);
#else
*/
    httplib::Client cli("localhost", 8080);
    DDWrite(cli, "./libc.so.6");
    gethashs("libc.so.6.hashs");
    DDRead(cli,hashs,"Blifton");
    if (auto res = cli.Get("/hi"))
    {
        cout << res->status << endl;
        cout << res->get_header_value("Content-Type") << endl;
        cout << res->body << endl;
    }
    else
    {
        cout << "error code: " << res.error() << std::endl;
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
        auto result = cli.get_openssl_verify_result();
        if (result)
        {
            cout << "verify error: " << X509_verify_cert_error_string(result) << endl;
        }
#endif
    }
    return 0;
}