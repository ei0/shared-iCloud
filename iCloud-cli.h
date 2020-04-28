#pragma once
#include<iostream>
#include<sstream>
#include<vector>
#include<string>
#include<unordered_map>
#include<boost/filesystem.hpp>
#include<boost/algorithm/string.hpp>
#include"httplib.h"


#include<boost/multiprecision/cpp_int.hpp>//大数
#include<boost/multiprecision/random.hpp>//随机数
#include<boost/multiprecision/miller_rabin.hpp>//素性检测


using std::cin;
using std::cout;
using std::endl;
#define NUMBER 256
static int64_t tr_or_fa = 0;//若因误差导致错误，由此标记报出，true_or_false

typedef boost::multiprecision::int128_t DataType;

struct key {
	DataType m_ekey;//加密密钥公钥
	DataType m_dkey;//解密密钥私钥
	DataType m_pkey;//n
};

class NumProcess {//数据处理工具
public:
	static DataType gcdEx(DataType a, DataType b, DataType* x, DataType* y);//扩展欧几里得
	static bool GetGcd(DataType data1, DataType data2);//最大公约数（更相减损法）
};
DataType NumProcess::gcdEx(DataType a, DataType b, DataType* x, DataType* y)
{
	if (b == 0)
	{
		*x = 1, * y = 0;
		return a;
	}
	else
	{
		DataType gcd = gcdEx(b, a % b, x, y);
		/* r = GCD(a, b) = GCD(b, a%b) */
		DataType t = *x;
		*x = *y;
		*y = t - a / b * *y;
		return gcd;
	}//扩展欧几里得
}//扩展欧几里得
bool NumProcess::GetGcd(DataType data1, DataType data2) {
	//clock_t start, end;
	//start = clock();
	DataType tmp = 1;
	while (tmp) {
		if (data1 > data2)
			tmp = data1 - data2;
		else
			tmp = data2 - data1;
		if (tmp) {
			data1 = data2;
			data2 = tmp;
		}
	}
	if (data1 == 1) {
		//end = clock();
		//cout << "最大公约数TIMEs：" << (end - start) / CLOCKS_PER_SEC << 's' << endl;
		return true;

	}
	else {
		//end = clock();
		//cout << "最大公约数TIMEs：" << (end - start) / CLOCKS_PER_SEC << 's' << endl;
		return false;
	}
}//最大公约数

class RSA {//加解密模块
	key m_key;
public:
	RSA(int);
	RSA() = default;
	RSA& operator=( key& s);
	//a^b%n
	void encipher(const char* filename, const char* fileout);//对文件加密
	//void decrypt(const char* filename, const char* fileout);//对文件解密
	DataType encipher(DataType data);//对数据加密
	DataType decrypt(DataType data);//对数据解密
	key getallkey();//返回公私密钥
private:
	//key GetKeys();
	//产生公钥私钥流程
	DataType GetPrime();//得到素数
	bool isprime(DataType data);//判断素数
	DataType GetPkey(DataType prime1, DataType prime2);//得到共有部分
	DataType GetOrla(DataType prime1, DataType prime2);//欧拉公式
	DataType GetEkey(DataType orla);//公钥的私有部分
	DataType GetDkey(DataType ekey, DataType orla);//私钥的私有部分
};
RSA& RSA:: operator = ( key& s) {
	this->m_key = s;
}
RSA::RSA(int) {//生成公钥私钥
	DataType prime1 = GetPrime();
	DataType prime2 = GetPrime();
	while (prime1 == prime2)
		prime2 = GetPrime();
	DataType orla = GetOrla(prime1, prime2);
	//cout << "OL OK!" << endl;
	m_key.m_pkey = GetPkey(prime1, prime2);
	//cout << "PK OK!" << endl;
	m_key.m_ekey = GetEkey(orla);
	//cout << "Ek OK!" << endl;
	m_key.m_dkey = GetDkey(m_key.m_ekey, orla);
	//cout << "DK OK!" << endl;
}
void RSA::encipher(const char* filename, const char* fileout) {//对文件进行加密
	std::ifstream fin(filename, std::ios::binary);
	std::ofstream fout(fileout, std::ios::binary);
	if (!fin.is_open()) {
		perror("input file open filed");
		return;
	}
	char* bufin = new char[NUMBER];
	DataType* bufout = new DataType[NUMBER];

	while (!fin.eof()) {
		fin.read(bufin, NUMBER);
		/*if (!fin.good()) {
			std::cout << "read file" << filename << "failed" << endl;
			return;
		}*/
		int curNum = fin.gcount();
		cout << "加密——读取了多少数" << curNum << endl;
		for (int i = 0; i < curNum; i++) {
			bufout[i] = encipher((DataType)bufin[i]);
		}
		fout.write((char*)bufout, curNum * sizeof(DataType));
		if (!fout.good()) {
			std::cout << "write file" << fileout << "failed" << endl;
			return;
		}
		tr_or_fa += curNum;
	}
	delete[] bufin;
	delete[] bufout;
	fin.close();
	fout.close();
}

//void RSA::decrypt(const char* filename, const char* fileout) {//对文件进行解密
//	std::ifstream fin(filename, std::ios::binary);
//	std::ofstream fout(fileout, std::ios::binary);
//	if (!fin.is_open()) {
//		perror("input file open filed");
//		return;
//	}
//
//	DataType* bufin = new DataType[NUMBER];
//	char* bufout = new char[NUMBER];
//	int64_t sum = 0;
//
//	while (!fin.eof()) {
//		fin.read((char*)bufin, NUMBER * sizeof(DataType));
//		/*if (!fin.good()) {
//			std::cout << "read file" << filename << "failed" << endl;
//			return;
//		}*/
//		int num = fin.gcount();
//		cout << " 解密——读取了多少" << num << endl;
//		num /= sizeof(DataType);
//		for (int i = 0; i < num; i++) {
//			bufout[i] = (char)decrypt(bufin[i]);
//		}
//		fout.write(bufout, num);
//		if (!fout.good()) {
//			std::cout << "write file" << fileout << "failed" << endl;
//			return;
//		}
//		sum += num;
//	}
//	if (sum != tr_or_fa) {
//		cout << endl;
//		cout << "由于高效的大数素性导致极小概率的出错，请重新生成密钥进行加密！" << endl;
//	}
//	delete[] bufin;
//	delete[] bufout;
//	fout.close();
//	fin.close();
//}
DataType RSA::encipher(DataType data) {//对数据进行加密
	//return (DataType)pow(data, ekey) % pkey;
	DataType Ai = data;
	DataType msgE = 1;
	while (m_key.m_ekey)
	{
		if (m_key.m_ekey & 1)
			msgE = (msgE * Ai) % m_key.m_pkey;
		m_key.m_ekey >>= 1;
		Ai = (Ai * Ai) % m_key.m_pkey;
	}
	return msgE;
}

DataType RSA::decrypt(DataType data) {//对数据进行解密
	DataType Ai = data;
	DataType msgE = 1;
	while (m_key.m_dkey)
	{
		if (m_key.m_dkey & 1)
			msgE = (msgE * Ai) % m_key.m_pkey;
		m_key.m_dkey >>= 1;
		Ai = (Ai * Ai) % m_key.m_pkey;
	}
	return msgE;
}
key RSA::getallkey() {//得到公钥私钥
	return m_key;
}

DataType RSA::GetPrime() {//得到一个素数
	//srand(time(NULL));
	//clock_t start, end;
	//start = clock();
	//cout << "GetPrime:";
	boost::random::mt19937 gen(time(NULL));
	boost::random::uniform_int_distribution<DataType> dist(0, DataType(1) << 256);
	DataType prime;
	while (1) {
		prime = dist(gen);
		//cout << "rand：" << endl;
		//cout << prime << endl;
		if (isprime(prime))
			break;
	}
	//end = clock();
	//cout << "Get prime time: " << (end - start) / CLOCKS_PER_SEC << 's' << endl;
	return prime;
}
bool RSA::isprime(DataType data) {//判断一个数是否为素数
	boost::random::mt11213b gen(time(NULL));
	if (miller_rabin_test(data, 25, gen)) {
		if (miller_rabin_test((data - 1) / 2, 25, gen))
		{
			//cout <<data<<' '<< "OK!"<<endl;
			return true;
		}
	}
	return false;

}
//n = pq
DataType RSA::GetPkey(DataType prime1, DataType prime2) {//公钥与私钥的共有部分的产生
	return prime1 * prime2;
}
//f(n) = (p-1)(q-1)
DataType RSA::GetOrla(DataType prime1, DataType prime2) {
	return (prime1 - 1) * (prime2 - 1);
}
DataType RSA::GetEkey(DataType orla) {//传入欧拉公式结果，公钥独立部分的产生
	//srand(time(NULL));
	boost::random::mt19937 gen(time(NULL));
	boost::random::uniform_int_distribution<DataType> dist(2, orla);
	DataType ekey;
	while (1) {
		//ekey = rand() % orla + 2;
		ekey = dist(gen);
		if (NumProcess::GetGcd(ekey, orla)) {//通过判断两数的最大公约数是否为1 -> 判断两数是否互质
			return ekey;
		}
	}
}
DataType RSA::GetDkey(DataType ekey, DataType orla) {//求解私钥中的私有部分
	DataType num1 = 0, num2 = 0;
	NumProcess::gcdEx(ekey, orla, &num1, &num2);
	return (num1 % orla + orla) % orla;
}



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
		body->resize(fsize);//136
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
static std::vector<key> ll;
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
		ll.resize(2);
		
		ll[1].m_ekey.canonical_value("2800184539151241323459302850986519715696144296685853760645308061690089643239390505532510398301299975653080926643555061777562659010979801224790136230284111 ");
		ll[1].m_dkey.canonical_value("4848885409058917908003723275348148197890811414817782413646116008224878949005812621343931312472794389189508589395764407986188205754412786062463069943447383");
		ll[1].m_pkey.canonical_value("10289463658890621396555237681703897973886645657445020967914139291777578539394862064493203695685843350766637666631750723224861999246094233648519449177500573");
		ll[2].m_ekey.canonical_value("245124682646042434807546867091401631788307028667941976202522118974507063266027540441650139739294652349760633678015831301951092583434383505608268391126991 ");
		ll[2].m_dkey.canonical_value("340627394131608108985506094338774545843461818240172731072491040564514734139420326557710074200702268074813438793028216892504701489196735471347791948088875 ");
		ll[2].m_pkey.canonical_value("1639844236106861396250381607852650751037179221768129045110696666876580374565466460372693523712719313270860193128700371414300054119798093117436254651869201");
		
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

				if (0) {
					RSA a;
					a = ll[1];
					std::string tmp = name + "rsa";
					a.encipher(name.c_str(), tmp.c_str());
					pathname = m_listen_dir + tmp;
				}

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