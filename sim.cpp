#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <unistd.h>
#include <algorithm>
using namespace std;

// -m: medicine.txt
// -s: strategy.txt
// -d: delete.txt
const double deltav[] = {-1.5, -1, -0.5, 0, 2, 4, 6}; // 调价表
const int n = 50; // 药品总数
struct medicine{
	double v; // 进价
	int t; // 过期时间
	int sold; // 是否已经卖出/扔掉
}a[100]; // 存储药品相关信息 -- 进价以及过期时间
vector<int>d[11]; // 每天扔掉哪些药
struct ty{
	int id, delta;
}day[11][11];
int index[11];

// 排序(按照售价(1)和过期时间(2))
bool cmp(ty x, ty y){
	if ((a[x.id].v + deltav[x.delta]) == (a[y.id].v + deltav[y.delta])) return a[x.id].t > a[y.id].t;
	return a[x.id].v + deltav[x.delta] < a[y.id].v + deltav[y.delta];
}

// 读取数据
void read(int argc, char* argv[]){
	int op, DAY, ID, DELTA;
	const char* opstring = "m:s:d:";
	while ((op = getopt(argc, argv, opstring)) != -1){
		FILE* fp = fopen(optarg, "r");
		if (fp == NULL) {
			cout << "文件: " << optarg << "打开失败";
			continue;
		}
		if (op == 'm'){
			for (int i = 0;i < n;i++){
				fscanf(fp, "%lf%d", &a[i].v, &a[i].t);
				a[i].sold = 0;
			}
		}
		else if (op == 's'){
			for (int i = 1;i <= 10;i++)
				for (int j = 1;j <= 10;j++){
					fscanf(fp, "%d,%d", &ID, &DELTA);
					if (ID != -1) day[i][++index[i]].id = ID, day[i][index[i]].delta = DELTA;
				}
		}
		else if (op == 'd'){
			while ((fscanf(fp, "%d%d", &DAY, &ID)) != EOF) d[DAY].push_back(ID);
		}
	}
}

// 模拟
void Simulation(){
	double profit = 0; // 利润
	for (vector<int>::iterator it = d[0].begin();it != d[0].end();it++) a[*it].sold = 1, profit -= a[*it].v; // 清空第0天的库存
	for (int i = 1;i <= 10;i++){ // 枚举天数
		// 收取药品的仓库管理费
		for (int j = 0;j < n;j++)
			if (a[j].sold != 1){
				if (a[j].t - i < 5) profit -= 1;
				else profit -= 0.5;
			}
		sort(day[i] + 1, day[i] + 1 + index[i], cmp);
		int cnt = 0;
		// 上架药品
		for (int j = 1;j <= index[i];j++)
			if (a[day[i][j].id].sold != 1){
				if (cnt < 3) profit += deltav[day[i][j].delta], cnt++, a[day[i][j].id].sold = 1;
				// 上架的药品
				if (a[day[i][j].id].t - i + 1 <= 5) profit += 1;
				else profit += 0.5;
			}
		// 扔掉药品
		for (vector<int>::iterator it = d[i].begin();it != d[i].end();it++){
			if (a[*it].sold != 1) a[*it].sold = 1, profit -= a[*it].v;
		}
		for (int j = 0;j < n;j++) if (a[j].sold != 1 && a[j].t == i) profit -= a[j].v;
	}
	cout << profit << endl;
}

int main(int argc, char* argv[]){
	for (int i = 0;i <= 10;i++) d[i].clear();
	memset(index, 0, sizeof(index));
	read(argc, argv); // 读取数据
	Simulation(); // 模拟药店卖药

	return 0;
}