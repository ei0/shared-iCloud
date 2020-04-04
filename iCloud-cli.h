#pragma once
#include<iostream>
#include<sstream>
#include<vector>
#include<string>
#include<unordered_map>
#include<boost/filesystem.hpp>
#include<boost/algorithm/string.hpp>
#include"httplib.h"

class FileUtil {
public:
	static bool Read(const std::string& name, std::string* body)
	{
		std::ifstream fs(name, std::ios::binary);
		if (!fs.is_open()) {
			std::cout << "open file" << name << "failed" << std::endl;
			return false;
		}
		int64_t fsize = boost::filesystem::file_size(name);
		body->resize(fsize);
		fs.read(&(*body)[0], fsize);
		if (!fs.good()) {
			std::cout << "read file" << name << "failed" << std::endl;
			return false;
		}
		fs.close();
		return true;
	}
	static bool Write(const std::string& name, const std::string& body)
	{
		std::ofstream ofs(name, std::ios::binary);
		if (!ofs.is_open()) {
			std::cout << "open file" << name << "failed" << std::endl;
			return false;
		}
		ofs.write(body.c_str(), body.size());
		if (!ofs.good()) {
			std::cout << "write file" << name << "failed" << std::endl;
			return false;
		}
		ofs.close();
		return true;
	}
};
class DataManager
{
private:
	std::string m_store_file;
	std::unordered_map<std::string, std::string> m_backup_list;
public:
	DataManager(const std::string& filename): m_store_file(filename){}
	bool Insert(const std::string& key, const std::string& val)
	{
		m_backup_list[key] = val;
		Storage();
		return true;
	}
	bool GetEtag(const std::string& key, std::string* val)//获取文件的etag信息
	{
		auto it = m_backup_list.find(key);
		if (it == m_backup_list.end())
			return false;
		*val = it->second;
		return true;
	}
	bool Storage()//文件存储
	{
		std::stringstream tmp;
		for (auto it = m_backup_list.begin(); it != m_backup_list.end(); ++it)
		{
			tmp << it->first << ' ' << it->second << "\r\n";
		}
		FileUtil::Write(m_store_file, tmp.str());
		return true;
	}
	bool InitLoad()//初始化加载原有数据
	{
		std::string body;
		if (!FileUtil::Read(m_store_file, &body))
		{
			return false;
		}
		std::vector<std::string> list;
		boost::split(list, body, boost::is_any_of("\r\n"), boost::token_compress_off);
		for (auto i : list)
		{
			size_t pos = i.find(" ");
			if (pos == std::string::npos) {
				continue;
			}
			std::string key = i.substr(0, pos);
			std::string val = i.substr(pos + 1);
			Insert(key, val);
		}
		return true;
	}
};

class CloudClient 
{
private:
	std::string m_srv_ip;
	uint16_t m_srv_port;
	std::string m_listen_dir;
	DataManager data_manage;
public:
	CloudClient(const std::string& filename,const std::string& store_file, const std::string& srv_ip, uint16_t srv_port):
		m_srv_ip(srv_ip),
		m_srv_port(srv_port),
		m_listen_dir(filename),//监听文件目录
		data_manage(store_file)//文件列表存储
		{}
	bool Start() 
	{
		data_manage.InitLoad();
		while(1)
		{
			std::vector<std::string> list;
			GetBackupFileList(&list);
			for (size_t i = 0; i < list.size(); i++) {
				std::string name = list[i];
				std::string pathname = m_listen_dir + name;
				std::cout << pathname << "is need yo backup" << std::endl;
				std::string body;
				FileUtil::Read(pathname, &body);
				httplib::Client cli(m_srv_ip.c_str(), m_srv_port);
				std::string req_path = "/" + name;
				auto rsp = cli.Put(req_path.c_str(), body, "application/octet-stream");
				if (rsp == NULL || (rsp != NULL && rsp->status != 200)) {
					std::cout << pathname << "backup failed" << std::endl;
					continue;
				}
				std::string etag;
				GetEtag(pathname, &etag);
				data_manage.Insert(name, etag);
				std::cout << pathname << "backup success" << std::endl;
			}
			Sleep(1000);
		}
		return true;
	}
	bool GetBackupFileList(std::vector<std::string>* list)//获取需要备份的文件列表
	{
		if (boost::filesystem::exists(m_listen_dir) == false)
			boost::filesystem::create_directory(m_listen_dir);
		boost::filesystem::directory_iterator begin(m_listen_dir);
		boost::filesystem::directory_iterator end;
		for (; begin != end; ++begin) {
			if(boost::filesystem::is_directory(begin->status()))
				continue;
			std::string pathname = begin->path().string();
			std::string name = begin->path().filename().string();
			std::string cur_etag;
			GetEtag(pathname, &cur_etag);
			std::string old_etag;
			data_manage.GetEtag(name, &old_etag);
			if (cur_etag != old_etag) {
				list->push_back(name);
			}
		}
		return true;
	}
	bool GetEtag(const std::string& pathname, std::string* etag)
	{
		int64_t fsize = boost::filesystem::file_size(pathname);
		time_t mtime = boost::filesystem::last_write_time(pathname);
		*etag = std::to_string(fsize) + "-" + std::to_string(mtime);
		return true;
	}
};