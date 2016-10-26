#include"Header.h"

int main() {
	FILE* RawTaxiData = fopen("yellow_tripdata_2016-01.csv", "r");
	if (RawTaxiData == NULL) {
		printf("Illegal TaxiData!\n");
		return 0;
	}
	FILE* RawRoadData = fopen("Centerline.csv", "r");
	if (RawRoadData == NULL) {
		printf("Illegal TaxiData!\n");
		return 0;
	}
	FILE*  OutputMap = fopen("Map.csv", "w");
	FILE*  OutputNode = fopen("Node.csv", "w");
	FILE*  OutputRoute = fopen("Route.csv", "w");
	DataProcess(RawTaxiData, RawRoadData, OutputMap, OutputNode ,OutputRoute);
	_fcloseall();
	return 0;
}