#pragma once
#include<stdio.h>
#include<map>
#include<vector>
using namespace std;
#define EPSILON 0.000001
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
};
void DataProcess(FILE*, FILE*, FILE*, FILE*, FILE*);
