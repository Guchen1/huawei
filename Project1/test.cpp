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
const static int MAX_CLIENT = 36; // ���Ŀͻ��ڵ�
const static int MAX_SITE = 135;  //���ı�Ե�ڵ�
int clientArrLen;
int siteArrLen;
int QOS_LIMIT; // ����ӳ�


// ����Ҫ��ȡÿ��ʱ������е��������м���ͨ�������ܹ�ȷ����Ӧ�Ŀͻ��ڵ�����ּ��ɡ�
vector<string> timeSeqName;//ʱ�����е�����
vector<vector<int> > demand;//ÿ��ʱ��ڵ�Ĵ�����������

/**
 * Site,���������еı�Ե�ڵ�
 */
struct SiteNode
{
    string siteName;
    int siteIndex;
    int bandwidth; //��Site�ɷ����������ޣ�������1000000MB
    int curAlloc;            //��ǰ�ýڵ���䵽������
    bool ifAllocSat(int cnt) // �Ƿ��������cnt����
    {
        return curAlloc + cnt <= bandwidth;
    }
    int hot = 0;   //�ȶ�
    int timeT = 0; //��ռ��ʱ��
} siteArr[MAX_SITE];
/**
 *
 * �ͻ��ڵ�
 */
struct ClientNode
{
    string clientName;
    vector<vector<pair<int, int>>> alloc; //ÿ��ʱ���site�ڵ���������������
} clientArr[MAX_CLIENT];

struct QosTable
{
    // ��ʼ��ÿ�����ӹ�ϵ��Ϊ���ֵ
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
 * ����һ�е�����vector
 */
static vector<int> split2(const string& s, const string& delimiters = ",")
{
    string str;
    vector<int> tokens;
    size_t lastPos = s.find_first_not_of(delimiters, 0);//���ҵ�һ����,
    size_t pos = s.find_first_of(delimiters, lastPos);//���ҵ�һ��,
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
    //��ʼ����Ե�ڵ��б��Լ�ÿ����Ե�ڵ�Ĵ�������
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

    //�ͻ��ڵ�ͱ�Ե�ڵ��Qos
    data.open(DATA_PATH + "qos.csv");
    getline(data, tmp_line);
    // cout << tmp_line << endl;
    tmp_vec = split(tmp_line, ",");
    for (index = 0; index < tmp_vec.size() - 1; index++)
    {
        clientArr[index].clientName = tmp_vec[index + 1];
    }
    clientArrLen = index;

    // ����ÿ���ڵ��Qos
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

    //�ͻ��ڵ��ڲ�ͬʱ�̵Ĵ���������Ϣ
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
} //���ȶ�����

void allocateBandwith(int timeIndex,ostream& fout) //��򻯰棬��δ����������ȶ���95%����ֹ
{
    for (int i = 0; i < clientArrLen; ++i)//��Ҫ�߼�
    {
        //TODO��д��ͻ�����
        fout << clientArr[i].clientName << ':';
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
        string temp;
        int shengYu=0;
        int tempD=demand[timeIndex][i]/(avIndex.size());

        for (auto j : avIndex)  //�����ܿɷ�������
        {
            if (siteArr[j].timeT>=timeSeqName.size()*0.05-1)
            {
                continue;
            }
            //TODO��д���Ե�ڵ�����
            int allocOrigin = siteArr[j].curAlloc;
            siteArr[j].curAlloc += tempD;
            if (j == avIndex.size() - 1)//���һ���ڵ�Ҫ�����еĴ�������
            {
                siteArr[j].curAlloc -= tempD;
                siteArr[j].curAlloc += demand[timeIndex][i] - (j - 1) * tempD;
            }
            if (siteArr[j].curAlloc > siteArr[j].bandwidth)//��������޶��ˣ��͸���һ���ڵ㣬ͬʱ��ռ��++
            {
                shengYu = siteArr[j].curAlloc - siteArr[j].bandwidth;
                siteArr[j + 1].curAlloc += shengYu;
                siteArr[j].timeT++;
            }
            if (siteArr[j].curAlloc > siteArr[j].bandwidth * 0.95)//�������0.95����ռ��++
            {
                siteArr[j].timeT++;
            }
            int alloc= siteArr[j].curAlloc-allocOrigin;//TODO:д�����
            temp += "<" + siteArr[j].siteName+ "," + to_string(alloc) + ">,";
        }
        temp.erase(temp.end()-1);
        temp += '\n';
        fout << temp;
    }
}

void allocateBandwith2(int timeIndex, ostream& fout) //ʣ��5%��ʱ��
{
    for (int i = 0; i < clientArrLen; ++i)//��Ҫ�߼�
    {
        //TODO��д��ͻ�����
        fout << clientArr[i].clientName << ':';
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
        
        string temp;
        for (auto j : avIndex)  //�����ܿɷ�������
        {
            if (siteArr[j].timeT >= timeSeqName.size() * 0.05 - 1)
            {
                continue;
            }
            //TODO��д���Ե�ڵ�����
            
            int allocOrigin = siteArr[j].curAlloc;
            siteArr[j].curAlloc += demand[timeIndex][i];
            if (siteArr[j].curAlloc > siteArr[j].bandwidth)//��������޶��ˣ��͸���һ���ڵ㣬ͬʱ��ռ��++
            {
                shengYu = siteArr[j].curAlloc - siteArr[j].bandwidth;
                siteArr[j + 1].curAlloc += shengYu;
                siteArr[j].timeT++;
            }
            if (siteArr[j].curAlloc > siteArr[j].bandwidth * 0.95)//�������0.95����ռ��++
            {
                siteArr[j].timeT++;
            }
            int alloc = siteArr[j].curAlloc - allocOrigin;//TODO:д�����
            temp += "<" + siteArr[j].siteName + "," + to_string(alloc) + ">,";
            break;
        }
        temp.erase(temp.end() - 1);
        temp += '\n';
        fout << temp;
    }
}


void testIO()
{
    readConf();
    readData();
}

int main()
{
    ofstream output(OUTPUT_PATH,ios::in);
    testIO();
    for (int j = 0; j < siteArrLen; ++j)    //����ȶ�
    {
        for (int i = 0; i < clientArrLen; ++i)
        {
            if (qosTable.ifQosSat(siteArr[j].siteIndex, i))
            {
                siteArr[j].hot++;
            }
        }
    }
    sort(siteArr, siteArr + siteArrLen, cmpHot); //���ȶ�����
    for (int time = 0; time < timeSeqName.size(); ++time) {
        if (time <= 0.95 * timeSeqName.size())
            allocateBandwith(time,output);
        else
            allocateBandwith2(time,output);
    }
   



    return 0;
}