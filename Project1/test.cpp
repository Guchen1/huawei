#include <iostream>
#include <fstream>
#include <vector>
#include <iterator>
#include <string>
#include <map>
#include <unordered_map>
#include <algorithm>
using namespace std;

static string CONFIG_PATH = "./data/config.ini";
static string DATA_PATH = "./data/";
static string OUTPUT_PATH = "./output/solution.txt";
const static int MAX_CLIENT = 36; // 最大的客户节点
const static int MAX_SITE = 135;  //最大的边缘节点
int clientArrLen;
int siteArrLen;
int QOS_LIMIT; // 最大延迟


// 主需要获取每个时间点所有的需求序列及其通过索引能够确定相应的客户节点的名字即可。
vector<string> timeSeqName;//时间序列的名字
vector<vector<int> > demand;//每个时间节点的贷款需求序列

/**
 * Site,即任务书中的边缘节点
 */
struct SiteNode
{
    string siteName;
    int siteIndex;
    int bandwidth; //该Site可分配的最大上限，不超过1000000MB
    int curAlloc;            //当前该节点分配到流量。
    bool ifAllocSat(int cnt) // 是否满足分配cnt流量
    {
        return curAlloc + cnt <= bandwidth;
    }
    int hot = 0;   //热度
    int timeT = 0; //高占用时间
} siteArr[MAX_SITE];
/**
 *
 * 客户节点
 */
struct ClientNode
{
    string clientName;
    vector<vector<pair<int, int>>> alloc; //每个时间点site节点分配的索引及流量
} clientArr[MAX_CLIENT];

struct QosTable
{
    // 初始化每个连接关系都为最大值
    int qosData[MAX_SITE][MAX_CLIENT];
    inline bool ifQosSat(int siteIndex, int clientIndex)
    {
        return qosData[siteIndex][clientIndex] < QOS_LIMIT;
    }
} qosTable;



/**
 * String split by delimiters
 */
static vector<string> split(const string& s, const string& delimiters = ",")
{
    vector<string> tokens;
    size_t lastPos = s.find_first_not_of(delimiters, 0);
    size_t pos = s.find_first_of(delimiters, lastPos);
    while (pos != string::npos || lastPos != string::npos)
    {
        tokens.emplace_back(s.substr(lastPos, pos - lastPos));
        lastPos = s.find_first_not_of(delimiters, pos);
        pos = s.find_first_of(delimiters, lastPos);
    }
    return tokens;
}
/**
 * 返回一行的数字vector
 */
static vector<int> split2(const string& s, const string& delimiters = ",")
{
    string str;
    vector<int> tokens;
    size_t lastPos = s.find_first_not_of(delimiters, 0);//查找第一个非,
    size_t pos = s.find_first_of(delimiters, lastPos);//查找第一个,
    str = s.substr(lastPos, pos - lastPos);
    lastPos = s.find_first_not_of(delimiters, pos);
    pos = s.find_first_of(delimiters, lastPos);
    while (pos != string::npos || lastPos != string::npos)
    {
        tokens.emplace_back(atoi(s.substr(lastPos, pos - lastPos).c_str()));
        lastPos = s.find_first_not_of(delimiters, pos);
        pos = s.find_first_of(delimiters, lastPos);
    }
    return tokens;
}

/**
 * read Config
 */
void readConf()
{
    ifstream config;
    config.open(CONFIG_PATH);
    string tmp_line;
    getline(config, tmp_line);
    getline(config, tmp_line);
    QOS_LIMIT = atoi(string(tmp_line.begin() + tmp_line.find('=') + 1, tmp_line.end()).c_str());
    config.close();
}
/**
 * read site_bandwidth.csv, qos.csv, demand.csv
 */
void readData()
{
    vector<string> tmp_vec;
    vector<int> tmp_vec2;
    ifstream data;
    string tmp_line;
    unsigned int index = 0;
    //初始化边缘节点列表以及每个边缘节点的带宽上限
    data.open(DATA_PATH + "site_bandwidth.csv");
    getline(data, tmp_line);
    // cout << tmp_line << endl;
    while (getline(data, tmp_line))
    {
        tmp_vec = split(tmp_line, ",");
        siteArr[index].siteName = tmp_vec[0];
        siteArr[index].bandwidth = atoi(tmp_vec[1].c_str());
        siteArr[index].curAlloc = 0;
        siteArr[index].siteIndex = index;
        index++;
    }
    siteArrLen = index;
    data.close();
    data.clear();

    //客户节点和边缘节点的Qos
    data.open(DATA_PATH + "qos.csv");
    getline(data, tmp_line);
    // cout << tmp_line << endl;
    tmp_vec = split(tmp_line, ",");
    for (index = 0; index < tmp_vec.size() - 1; index++)
    {
        clientArr[index].clientName = tmp_vec[index + 1];
    }
    clientArrLen = index;

    // 处理每个节点的Qos
    index = 0;
    while (getline(data, tmp_line))
    {
        tmp_vec2 = move(split2(tmp_line, ","));
        for (unsigned int k = 0; k < tmp_vec2.size(); ++k)
        {
            qosTable.qosData[index][k] = tmp_vec2[k];
        }
        index++;
    }
    data.close();
    data.clear();

    //客户节点在不同时刻的带宽需求信息
    data.open(DATA_PATH + "demand.csv");
    getline(data, tmp_line);
    index = 0;
    while (getline(data, tmp_line))
    {
        timeSeqName.push_back(tmp_line.substr(0, tmp_line.find_first_of(",")));
        demand.push_back(split2(tmp_line, ","));
    }
    data.close();
    data.clear();
}

bool cmpHot(const SiteNode& a, const SiteNode& b)
{
    return a.hot > b.hot;
} //按热度排序

void allocateBandwith(int timeIndex) //最简化版，暂未考虑溢出，热度与95%的阻止
{
    for (int i = 0; i < clientArrLen; ++i)//主要逻辑
    {
        //TODO：写入客户名字
        if (demand[timeIndex][i] == 0)
        {
            continue;
        }
        vector<pair<int, int>> allocTemp;
        vector<int> avIndex;
        for (int j = 0; j < siteArrLen; ++j)
        {
            if (qosTable.ifQosSat(siteArr[j].siteIndex, i))
            {
                avIndex.push_back(j);
            }

        }
        
        int shengYu=0;
        int tempD=demand[timeIndex][i]/(avIndex.size());

        for (auto j : avIndex)  //计算总可分配流量
        {
            if (siteArr[j].timeT>=timeSeqName.size()*0.05-1)
            {
                continue;
            }
            //TODO：写入边缘节点名字
            int allocOrigin = siteArr[j].curAlloc;
            siteArr[j].curAlloc += tempD;
            if (j == avIndex.size() - 1)//最后一个节点要把所有的带宽都补上
            {
                siteArr[j].curAlloc -= tempD;
                siteArr[j].curAlloc += demand[timeIndex][i] - (j - 1) * tempD;
            }
            if (siteArr[j].curAlloc > siteArr[j].bandwidth)//如果超出限额了，就给下一个节点，同时高占用++
            {
                shengYu = siteArr[j].curAlloc - siteArr[j].bandwidth;
                siteArr[j + 1].curAlloc += shengYu;
                siteArr[j].timeT++;
            }
            if (siteArr[j].curAlloc > siteArr[j].bandwidth * 0.95)//如果超出0.95，高占用++
            {
                siteArr[j].timeT++;
            }
            int alloc= siteArr[j].curAlloc-allocOrigin;//TODO:写入带宽
        }
    }
}

void allocateBandwith2(int timeIndex) //剩余5%的时间
{
    for (int i = 0; i < clientArrLen; ++i)//主要逻辑
    {
        //TODO：写入客户名字
        if (demand[timeIndex][i] == 0)
        {
            continue;
        }
        vector<pair<int, int>> allocTemp;
        vector<int> avIndex;
        for (int j = 0; j < siteArrLen; ++j)
        {
            if (qosTable.ifQosSat(siteArr[j].siteIndex, i))
            {
                avIndex.push_back(j);
            }

        }

        int shengYu = 0;
        

        for (auto j : avIndex)  //计算总可分配流量
        {
            if (siteArr[j].timeT >= timeSeqName.size() * 0.05 - 1)
            {
                continue;
            }
            //TODO：写入边缘节点名字
            int allocOrigin = siteArr[j].curAlloc;
            siteArr[j].curAlloc += demand[timeIndex][i];
            if (siteArr[j].curAlloc > siteArr[j].bandwidth)//如果超出限额了，就给下一个节点，同时高占用++
            {
                shengYu = siteArr[j].curAlloc - siteArr[j].bandwidth;
                siteArr[j + 1].curAlloc += shengYu;
                siteArr[j].timeT++;
            }
            if (siteArr[j].curAlloc > siteArr[j].bandwidth * 0.95)//如果超出0.95，高占用++
            {
                siteArr[j].timeT++;
            }
            int alloc = siteArr[j].curAlloc - allocOrigin;//TODO:写入带宽
            break;
        }
    }
}


void testIO()
{
    readConf();
    readData();
}

int main()
{
    testIO();
    for (int j = 0; j < siteArrLen; ++j)    //获得热度
    {
        for (int i = 0; i < clientArrLen; ++i)
        {
            if (qosTable.ifQosSat(siteArr[j].siteIndex, i))
            {
                siteArr[j].hot++;
            }
        }
    }
    sort(siteArr, siteArr + siteArrLen, cmpHot); //按热度排序
    int hotNum = 0.3 * siteArrLen;  //设置热度阈值
    for (int i = 0; i <= hotNum; ++i)  //设置热度
    {
        siteArr[i].hot = 1;
    }
    for (int i = hotNum + 1; i < siteArrLen; ++i)
    {
        siteArr[i].hot = 0;
    }



    return 0;
}