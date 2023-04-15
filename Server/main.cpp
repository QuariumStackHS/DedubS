
#include "Dedupmf.hpp"
#include <iostream>
#include <vector>
#include <dirent.h>
#include <sstream>
using namespace std;
#include "../common/httplib/httplib.hpp"
using namespace httplib;
string listall()
{
    stringstream ss;
    DIR *dir;
    struct dirent *diread;
    vector<char *> files;

    if ((dir = opendir("./Storage/Index")) != nullptr)
    {
        while ((diread = readdir(dir)) != nullptr)
        {
            files.push_back(diread->d_name);
        }
        closedir(dir);
    }
    else
    {
        perror("opendir");
        return "Nond";
    }
    int nbr = 0;
    for (auto file : files)
        if (nbr < 2)
            nbr++;
        else
            ss << file << ",";
    return ss.str();
}
std::string dump_headers(const Headers &headers)
{
    std::string s;
    char buf[BUFSIZ];

    for (auto it = headers.begin(); it != headers.end(); ++it)
    {
        const auto &x = *it;
        snprintf(buf, sizeof(buf), "%s: %s\n", x.first.c_str(), x.second.c_str());
        s += buf;
    }

    return s;
}

std::string log(const Request &req, const Response &res)
{
    std::string s;
    char buf[BUFSIZ];

    s += "================================\n";

    snprintf(buf, sizeof(buf), "%s %s %s", req.method.c_str(),
             req.version.c_str(), req.path.c_str());
    s += buf;

    std::string query;
    for (auto it = req.params.begin(); it != req.params.end(); ++it)
    {
        const auto &x = *it;
        snprintf(buf, sizeof(buf), "%c%s=%s",
                 (it == req.params.begin()) ? '?' : '&', x.first.c_str(),
                 x.second.c_str());
        query += buf;
    }
    snprintf(buf, sizeof(buf), "%s\n", query.c_str());
    s += buf;

    s += dump_headers(req.headers);

    s += "--------------------------------\n";

    snprintf(buf, sizeof(buf), "%d %s\n", res.status, res.version.c_str());
    s += buf;
    s += dump_headers(res.headers);
    s += "\n";

    if (!res.body.empty())
    {
        s += res.body;
    }

    s += "\n";

    return s;
}
string Packages;
string get_command(string path)
{
    string k = "";
    for (int i = 1; path[i] != '/'; i++)
    {
        k.push_back(path[i]);
    }
    return k;
}
string get_args(string path)
{
    string k = "";
    int start = 0;
    for (int i = 1; path[i] != '/'; i++)
    {
        start = i + 1;
    }
    for (int i = start; i < path.size(); i++)
    {
        k.push_back(path[i]);
    }
    return k;
}
string getstringfrompos(string a, int pos)
{
    string tempa = a;
    tempa += "/";
    if (pos == 0)
    {
        return get_command(a);
    }
    else
    {
        for (int i = 0; i < pos; i++)
        {
            tempa = get_args(tempa);
        }
        if (get_command(tempa)[0] == '/')
        {
            string TTA = "";
            for (int j = 1; j < tempa.size(); j++)
                TTA.push_back(tempa[j]);
            return TTA;
        }
    }

    return get_command(tempa);
} /*
 file server
 file struct:
 */
void list_directory(string path, vector<string> &A)
{
    A.clear();
    for (const auto &entry : fs::directory_iterator(path))
        A.push_back(entry.path());
}
Datadedup *DD;
DDindex *DDI;
int main(void)
{
    DD = new Datadedup("Storage/Blocks/");
    DDI = new DDindex("Storage/Index/");
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    SSLServer svr(SERVER_CERT_FILE, SERVER_PRIVATE_KEY_FILE);
#else
    Server svr;
#endif
    ifstream PackageList("P.List");
    stringstream ss;
    ss << PackageList.rdbuf();
    Packages = ss.str();
    if (!svr.is_valid())
    {
        printf("server has an error...\n");
        return -1;
    }

    svr.Get("/", [=](const Request & /*req*/, Response &res)
            { res.set_redirect("/hi"); });
    svr.Get("/deps", [=](const Request & /*req*/, Response &res)
            {
    ifstream PackageList("P.List");
    stringstream ss;
    ss<<PackageList.rdbuf();
    Packages=ss.str();
    res.set_content(Packages,"text/plain"); });

    svr.Get("/stop", [&](const Request &req, Response &res)
            { svr.stop(); });
    svr.set_error_handler([](const Request &req, Response &res)
                          {
                            string commandparser=req.path;
                            stringstream SSResponse;
                              if (strcmp(get_command(commandparser).c_str(), "read")==0)
                              {
                                string hash=getstringfrompos(commandparser,1);
                                SSResponse<<DD->read_packet(hash);
                              }
                            if (strcmp(get_command(commandparser).c_str(), "write")==0)
                              {
                                string hash=getstringfrompos(commandparser,1);
                                string packet=getstringfrompos(commandparser,2);
                                DD->write_packet(packet,hash);
                                SSResponse<<"Done";
                                //SSResponse<<getstringfrompos(commandparser,1)<<"-->"<<get_args(get_args(commandparser))<<endl;
                              }
                              if(strcmp(get_command(commandparser).c_str(),"list")==0){
                                SSResponse<<listall();
                              }
                              //download on server (upload for client)
                              if(strcmp(get_command(commandparser).c_str(), "exists")==0){
                                SSResponse<<exists("Storage/Blocks/"+getstringfrompos(commandparser,1));
                              }
                              if(strcmp(get_command(commandparser).c_str(), "append")==0){
                                DDI->index_append(getstringfrompos(commandparser,1),getstringfrompos(commandparser,2));
                              }
                            if(strcmp(get_command(commandparser).c_str(), "download")==0){
                                SSResponse<<DDI->index_get(getstringfrompos(commandparser,1));
                                //cout<<"i"<<endl;
                              }
                            if(strcmp(get_command(commandparser).c_str(), "remove")==0){
                                remove(("Storage/Index/"+getstringfrompos(commandparser,1)).c_str());
                                //cout<<"i"<<endl;
                              }
                              res.set_content(SSResponse.str(), "text/plain"); });

    svr.set_logger([](const Request &req, const Response &res)
                   { printf("%s", log(req, res).c_str()); });
    cout << "listening" << endl;
    svr.listen("localhost", 8080);

    return 0;
}
