#pragma once
#include<stdio.h>
#include<map>
#include<vector>
#include<cmath>
using namespace std;
#define EPSILON 0.0000001
inline bool lequal(double a, double b) {
	return (b - a) > EPSILON;
}
inline bool equal(double a, double b) {
	return (b - a) < EPSILON && (b - a) > -EPSILON;
}
struct NODE {
	double x, y;
	NODE(double x,double y):x(x),y(y){}
	friend bool operator<(const NODE& a, const NODE&b) {
		return lequal(a.x, b.x) || equal(a.x, b.x) && lequal(a.y, b.y);
	}
	friend bool operator==(const NODE&a, const NODE&b) {
		return equal(a.x, b.x) && equal(a.y, b.y);
	}
	double Dis(const NODE& t) {
		return sqrt((x - t.x)*(x - t.x) + (y - t.y)*(y - t.y));
	}
};
void DataProcess(FILE*, FILE*, FILE*, FILE*, FILE*);
