#include"Header.h"
int FindForward(map<NODE, int>& Order, map<NODE, int>::iterator& IterBase, NODE& node,int& MinDistance) {
	int ans;
	map<NODE, int>::iterator iter = IterBase;
	for (int i = 0; i < 100; i++) {
		if (++iter != Order.end()) {
			if (node.Dis(iter->first) < MinDistance)
				MinDistance = node.Dis(iter->first);
			ans = iter->second;
		}
		else
			break;
	}
	return ans;
}
int FindBack(map<NODE, int>& Order, map<NODE, int>::iterator& IterBase, NODE& node, int& MinDistance) {
	int ans;
	map<NODE, int>::iterator iter = IterBase;
	for (int i = 0; i < 100; i++) {
		if (iter-- != Order.begin()) {
			if (node.Dis(iter->first) < MinDistance)
				MinDistance = node.Dis(iter->first);
			ans = iter->second;
		}
		else
			break;
	}
	return ans;
}
void DataProcess(FILE* taxi, FILE* road, FILE* omap, FILE* onode, FILE* oroute) {
	fscanf(road, "L_LOW_HN,the_geom,PHYSICALID,L_HIGH_HN,R_LOW_HN,R_HIGH_HN,L_ZIP,R_ZIP,L_BLKFC_ID,R_BLKFC_ID,ST_NAME,STATUS,BIKE_LANE,BOROCODE,ST_WIDTH,CREATED,MODIFIED,TRAFDIR,RW_TYPE,FRM_LVL_CO,TO_LVL_CO,SNOW_PRI,SHAPE_Leng\n");
	fscanf(taxi, "VendorID,tpep_pickup_datetime,tpep_dropoff_datetime,passenger_count,trip_distance,pickup_longitude,pickup_latitude,RatecodeID,store_and_fwd_flag,dropoff_longitude,dropoff_latitude,payment_type,fare_amount,extra,mta_tax,tip_amount,tolls_amount,improvement_surcharge,total_amount\n");
	map<NODE, int> Order;
	map<NODE, int>::iterator iter;
	int Num_node = 0;
	int n = 0;
	fprintf(omap, "na,nb,road_length,road_width\n");
	fprintf(onode, "node,x,y\n");
	fprintf(oroute, "pickup_time,dropoff_time,pickup_node,dropoff_node,passenger_count,trip_distance\n");
	while (!feof(road)) {
		double ax, ay, bx, by;
		int width;
		double length;
		char route[50000];
		char tmp[50000];
		fscanf(road, "%*[^,],\"LINESTRING (%[^)])\"", route);
		int len = strlen(route);
		for (int i = 0; i < 12; i++)
			fscanf(road, ",%[^,]", tmp);
		fscanf(road, ",%d",&width);
		for (int i = 0; i < 7; i++)
			fscanf(road, ",%*[^,]");
		fscanf(road, ",%lf", &length);

		sscanf(route, "%lf %lf", &bx, &by);
		iter = Order.find(NODE(bx, by));
		int na, nb;
		if (iter == Order.end()) {
			Order.insert(pair<NODE, int>(NODE(bx, by), Num_node));
			nb = Num_node++;
			fprintf(onode, "%d,%lf,%lf\n", nb, bx, by);
		}
		else
			nb = iter->second;
		char* pch = strchr(route, ',');
		while (pch != NULL)
		{										 
			ax = bx;
			ay = by;
			na = nb;
			sscanf(pch + 1, "%lf %lf", &bx, &by);
			iter = Order.find(NODE(bx, by));
			if (iter == Order.end()) {
				Order.insert(pair<NODE, int>(NODE(bx, by), Num_node));
				nb = Num_node++;
				fprintf(onode, "%d,%lf,%lf\n", nb, bx, by);
			}
			else
				nb = iter->second;
			pch = strchr(pch + 1, ',');
		}
		n++;
		if (n % 10000 == 0)
			printf("Processing No.%d.....\n", n);
		fprintf(omap, "%d,%d,%lf,%d\n", na, nb, length, width);
	}
	n = 0;
	return;
	while (!feof(taxi)) {
		char pickup[100];
		char dropoff[100];
		int passenger_count;
		int MinDistance;
		double trip_distance;
		double ax, ay, bx, by;
		fscanf(taxi, "%*[^,],%[^,],%[^,],", pickup, dropoff);
		fscanf(taxi, "%d,%lf,%lf,%lf,%*[^,],%*[^,],%lf,%lf,%*[^\n]\n", &passenger_count, &trip_distance, &ax, &ay, &bx, &by);
		NODE a(ax, ay);	  
		NODE b(bx, by);
		map<NODE, int>::iterator itera = Order.lower_bound(a);
		map<NODE, int>::iterator iterb = Order.lower_bound(b);
		MinDistance = 100000;
		int na = FindForward(Order, itera, a, MinDistance);
		na = FindBack(Order, itera, a, MinDistance);
		MinDistance = 100000;
		int nb = FindForward(Order, iterb, a, MinDistance);
		nb = FindBack(Order, iterb, a, MinDistance);
		fprintf(oroute, "%s,%s,%d,%d,%d,%lf\n", pickup, dropoff, na, nb, passenger_count, trip_distance);
		n++;
		if (n % 10000 == 0)
			printf("Processing No.%d.....\n", n);
	}
}