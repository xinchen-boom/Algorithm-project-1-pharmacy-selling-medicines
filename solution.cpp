#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <unistd.h>
using namespace std;

// 注意: 需要扔掉的药品必须选择在第1天就扔掉
const double deltav[] = { -1.5, -1, -0.5, 0, 2, 4, 6 }; // 调价表
int n = 50; // 药品总数
int s_a[100]; // 过期时间升序的药品编号
struct medicine {
	double v;
	int t;
}a[100]; // 存储药品相关信息 -- 进价以及过期时间

// 模拟退火相关结构体
struct SAmedicine {
	int A[11][3]; // 每天卖出的三种药品
	int n; // 存储剩余的药品的数量
	int B[50]; // 存储剩余的药品(不会丢掉, 会放置在仓库内)
	SAmedicine() : n(0) {}
}initstate, bestSAstr;
int initDel[100], initDelcnt; // 需要扔掉的药品的编号及其数量
double initCost; // 扔药的损失
double bestans = -1e5; // 最大获利
vector<int>medicine1, medicine2; // medcine1是即将过期(<= 5)的药; medicine2为其余(过期时间较长)药(> 5) (过期时间不同, 管理费不同)
string pathStrategy, pathDelete;

// 策略结构体, 包含strategy和delete
struct Strategy {
	int s[15][15][2]; // 营销策略 strategy
	/*
	s[i][j][0] -- 第i天第j种药品的编号
	s[i][j][1] -- 第i天第j种药品的调价
	*/
	int d[51][2]; // 要丢弃的药 delete
	/*
	d[i][0] -- 被丢弃的天数
	d[i][1] -- 被丢弃的药的编号
	*/
	int n; // 将被丢弃的药的数量

	Strategy(){
		n = 0;
	}

	Strategy& operator = (const Strategy& b) {
		memcpy(s, b.s, sizeof(s)); memcpy(d, b.d, sizeof(d));
		n = b.n;
		return *this;
	}

	void print() {
		ofstream fout;
		fout.open(pathStrategy.c_str());
		for (int i = 1; i <= 10; i++) {
			for (int j = 0; j < 10; j++) fout << s[i][j][0] << ", " << s[i][j][1] << "\t";
			fout << "\n";
		}
		fout.close();
		fout.open(pathDelete.c_str());
		for (int i = 0; i < n; i++) fout << d[i][0] << " " << d[i][1] << "\n";
		fout.close();
	}
}beststr;

// 药品进价升序排序
bool cmp(int x, int y) {
	return a[x].v > a[y].v;
}

// 过期时间降序排序
bool cmpDay(int x, int y) {
	if (a[x].t != a[y].t) return a[x].t < a[y].t;
	return a[x].v < a[y].v;
}

// 读入数据
void read(int argc, char* argv[]) {
	int op;
	const char* optstring = "m:s:d:";
	while ((op = getopt(argc, argv, optstring)) != -1) {
		FILE* fp = fopen(optarg, "r");
		if (op == 'm')
			for (int i = 0; i < n; i++) {
				fscanf(fp, "%lf%d", &a[i].v, &a[i].t);
				s_a[i] = i;
			}
		else if (op == 's') pathStrategy = optarg;
		else if (op == 'd') pathDelete = optarg;
		fclose(fp);
	}
}

// 贪心生成初始解
SAmedicine greedy() {
	int p = 0;
	SAmedicine ret;
	for (int i = 1; i <= 10; i++)
		for (int j = 0; j < 3; j++) {
			while (a[s_a[p]].t < i) {
				int u = s_a[p]; // 药品编号
				for (int m = 1; m <= a[u].t; m++)
					for (int n = 0; n < 3; n++)
						if (a[ret.A[m][n]].v < a[u].v) swap(u, ret.A[m][n]);
				initDel[initDelcnt++] = u, initCost += a[u].v, p++; // 在初始贪心时确定需要丢弃的药品
			}
			ret.A[i][j] = s_a[p++];
		}
	while (p < 50) ret.B[ret.n++] = s_a[p++]; // 多余的药品(不丢弃)
	return ret;
}

// 计算盈利
double caculate(SAmedicine s) {
	Strategy str; // 营销策略
	double profit = 0.0;
	medicine1.clear(), medicine2.clear(); // 清空仓库
	for (int i = 0; i < s.n; i++) { // 枚举剩余的药品(需要放在仓库进行保存)
		int u = s.B[i];
		if (a[u].t <= 10) str.d[str.n][0] = 0, str.d[str.n][1] = u, str.n++, profit -= a[u].v;
		else medicine2.push_back(u);
	}
	for (int i = 1; i <= 10; i++)
		for (int j = 0; j < 3; j++) {
			if (a[s.A[i][j]].t <= 5) medicine1.push_back(s.A[i][j]);
			else medicine2.push_back(s.A[i][j]);
		}

	// 模拟药店每天卖药
	for (int i = 1; i <= 10; i++) {
		sort(s.A[i], s.A[i] + 3, cmp); //按照进价升序排序
		double maxv = a[s.A[i][0]].v; // 当天售卖的三种药品中进价最高的药品
		// 将临近过期的药转移到 medcine1(以便于收取不同的管理费)
		for (vector<int>::iterator it = medicine2.begin(); it != medicine2.end();) {
			int u = *it;
			if (a[u].t - i + 1 <= 5) medicine1.push_back(u), it = medicine2.erase(it);
			else it++;
		}
		// 上架要卖的三种药
		for (int j = 0; j < 3; j++) {
			int u = s.A[i][j];
			str.s[i][j][0] = u;
			if (a[u].t - i + 1 <= 5) medicine1.erase(find(medicine1.begin(), medicine1.end(), u));
			else medicine2.erase(find(medicine2.begin(), medicine2.end(), u));
		}
		sort(medicine1.begin(), medicine1.end(), cmp);
		sort(medicine2.begin(), medicine2.end(), cmp);
		profit -= (medicine1.size() + medicine2.size() * 0.5); // 先减去仓库里所有药(除去当天必定上架的三种药)的管理费

		int p = 0, q = 0;
		int k0 = 6, k1 = 6, k2 = 6, u = s.A[i][0], v = s.A[i][1], w = s.A[i][2];
		int maxk0, maxk1, maxk2, maxp, maxq;
		double Max = -1e5;
		// 假设即将卖出的三种药品的定价不超过maxv + k, 计算当天的收益(卖药的利润＋上架的药品的仓库管理费)
		for (int k = 6; k >= -1; k--) {
			while (p < medicine1.size() && a[medicine1[p]].v + 6 > maxv + k) p++;
			while (q < medicine2.size() && a[medicine2[q]].v + 6 > maxv + k) q++;
			while (a[u].v + deltav[k0] > maxv + k) k0--;
			while (a[v].v + deltav[k1] > maxv + k) k1--;
			while (a[w].v + deltav[k2] > maxv + k) k2--;
			double tmp = deltav[k0] + deltav[k1] + deltav[k2] + min(p, 7) + 0.5 * min(q, 7 - min(p, 7));
			if (tmp > Max) Max = tmp, maxp = p, maxq = q, maxk0 = k0, maxk1 = k1, maxk2 = k2;
		}
		int t = 0;
		str.s[i][0][1] = maxk0, str.s[i][1][1] = maxk1, str.s[i][2][1] = maxk2;
		while (t < 7 && maxp) str.s[i][3 + t][0] = medicine1[--maxp], str.s[i][3 + t][1] = 6, t++;
		while (t < 7 && maxq) str.s[i][3 + t][0] = medicine2[--maxq], str.s[i][3 + t][1] = 6, t++;
		while (t < 7) str.s[i][3 + t][0] = -1, str.s[i][3 + t][1] = 6, t++;
		profit += Max;
	}
	if (profit > bestans) bestans = profit, beststr = str, bestSAstr = s;
	return profit;
}

// 调整退火的新状态
SAmedicine change(SAmedicine s) {
	for (int i = 0; i <= rand() % 5; i++)
		while (1) {
			int op = rand() % 4;
			if (op == 0) { // 与剩余的药品进行交换
				int x = rand() % 10 + 1, y = rand() % 3, z = rand() % s.n;
				if (a[s.B[z]].t >= x && a[s.A[x][y]].t > 10) {
					swap(s.B[z], s.A[x][y]);
					break;
				}
			}
			else { // 内部交换顺序
				int x = rand() % 10 + 1, y = rand() % 3, u = rand() % 10 + 1, v = rand() % 3;
				if (a[s.A[x][y]].t >= u && a[s.A[u][v]].t >= x) {
					swap(s.A[x][y], s.A[u][v]);
					break;
				}
			}
		}
	return s;
}

// 模拟退火求解
void SA() {
	double T = 10000, dT = 0.993, eps = 1e-6;
	SAmedicine state = initstate;
	double ans = caculate(state);
	while (T > eps) {
		SAmedicine newSate = change(state);
		double tmp = caculate(newSate), delta = tmp - ans;
		if (delta > 0 || exp(delta / T) * RAND_MAX > rand()) ans = tmp, state = newSate;
		T *= dT;
	}
}

// 主函数
int main(int argc, char* argv[]) {
	// 随机数种子
	srand(time(0) + 9927);

	// 读取数据
	read(argc, argv);

	// 将药品按照过期时间升序排列
	sort(s_a, s_a + n, cmpDay);
	// 贪心求解初始解
	initstate = greedy();
	// 模拟退火求解
	for (int i = 1; i <= 300; i++) SA();
	// 提高所求得的解的精度
	SAmedicine state = bestSAstr;
	double ans = bestans;
	for (int i = 1; i <= 5000; i++) {
		SAmedicine newState = change(state);
		double ansTmp = caculate(newState);
		if (ansTmp > ans) ans = ansTmp, state = newState;
	}
	for (int i = 0; i < initDelcnt; i++) beststr.d[beststr.n][0] = 0, beststr.d[beststr.n][1] = initDel[i], beststr.n++;
	beststr.print();
	cout << bestans - initCost << endl;

	return 0;
}