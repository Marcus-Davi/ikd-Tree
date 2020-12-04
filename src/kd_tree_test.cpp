#include <pcl/kdtree/kdtree_flann.h>
#include <kd_tree/kd_tree.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <random>
#include <algorithm>
#include <chrono>

#define X_MAX 1
#define X_MIN -1
#define Y_MAX 1
#define Y_MIN -1
#define Z_MAX 1
#define Z_MIN 0

#define Point_Num 100000
#define New_Point_Num 200
#define Delete_Point_Num 200
#define Nearest_Num 5
#define Test_Time 10000
#define Search_Time 400

vector<PointType> point_cloud;
vector<PointType> cloud_increment;
vector<PointType> cloud_decrement;
vector<PointType> search_result;
vector<PointType> raw_cmp_result;

pcl::KdTreeFLANN<PointType>::Ptr kdtreeSurfFromMap(new pcl::KdTreeFLANN<PointType>());

KD_TREE scapegoat_kd_tree(0.5,0.75);

float rand_float(float x_min, float x_max){
    float rand_ratio = rand()/(float)RAND_MAX;
    return (x_min + rand_ratio * (x_max - x_min));
}

void generate_initial_point_cloud(int num){
    vector<PointType> ().swap(point_cloud);
    PointType new_point;
    for (int i=0;i<num;i++){
        new_point.x = rand_float(X_MIN, X_MAX);
        new_point.y = rand_float(Y_MIN, Y_MAX);
        new_point.z = rand_float(Z_MIN, Z_MAX);
        point_cloud.push_back(new_point);
    }
    return;
}

void generate_increment_point_cloud(int num){
    vector<PointType> ().swap(cloud_increment);
    PointType new_point;
    for (int i=0;i<num;i++){
        new_point.x = rand_float(X_MIN, X_MAX);
        new_point.y = rand_float(Y_MIN, Y_MAX);
        new_point.z = rand_float(Z_MIN, Z_MAX);
        point_cloud.push_back(new_point);
        cloud_increment.push_back(new_point);        
    }
    return;
}

PointType generate_target_point(){
    PointType point;
    point.x = 0.0;
    point.y = 0.0;
    point.z = 0.0;
    return point;
}

void generate_decrement_point_cloud(int num){
    vector<PointType> ().swap(cloud_decrement);
    auto rng = default_random_engine();
    shuffle(point_cloud.begin(), point_cloud.end(), rng);    
    for (int i=0;i<num;i++){
        cloud_decrement.push_back(point_cloud[point_cloud.size()-1]);
        point_cloud.pop_back();
    }
    return;
}

void print_point_vec(vector<PointType> vec){
    printf("Size is %d\n", int(vec.size()));
    for (int i=0;i<vec.size();i++){
        printf("(%0.3f, %0.3f, %0.3f)\n",vec[i].x,vec[i].y,vec[i].z);
    }
    return;
}


void raw_cmp(PointType target, int k_nearest){
    vector<PointType_CMP> points;
    vector<PointType> ().swap(raw_cmp_result);
    float dist;
    for (int i=0;i<point_cloud.size();i++){
        dist = (point_cloud[i].x-target.x)*(point_cloud[i].x-target.x) + (point_cloud[i].y-target.y)*(point_cloud[i].y-target.y) + (point_cloud[i].z-target.z)*(point_cloud[i].z-target.z);
        PointType_CMP tmp{point_cloud[i],dist};
        points.push_back(tmp);
    }
    sort(points.begin(),points.end());
    for (int i=0;i<k_nearest;i++){
        raw_cmp_result.push_back(points[i].point);
    }
    return;
}

bool cmp_point_vec(vector<PointType> a, vector<PointType> b){
    if (a.size() != b.size()) return false;
    for (int i =0;i<a.size();i++){
        if (fabs(a[i].x-b[i].x)>EPS || fabs(a[i].y-b[i].y)>EPS || fabs(a[i].y-b[i].y)>EPS) return false;
    }
    return true;
}

int main(int argc, char** argv){
    srand((unsigned) time(NULL));
    printf("Testing ...\n");
    int counter = 0;
    bool flag = true;
    PointType target; 
    // Initialize k-d tree
    generate_initial_point_cloud(Point_Num);
    scapegoat_kd_tree.Build(point_cloud);    
    while (flag && counter < Test_Time){
        printf("Test %d:\n",counter+1);
        // Incremental Operation
        generate_increment_point_cloud(New_Point_Num);
        auto t1 = chrono::high_resolution_clock::now();
        scapegoat_kd_tree.Add_Points(cloud_increment, New_Point_Num);
        auto t2 = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::microseconds>(t2-t1).count();
        auto total_duration = duration;
        printf("Add point time cost is %0.3f ms\n",float(duration)/1e3);
        // Decremental Operation
        duration = duration - duration;   
        generate_decrement_point_cloud(Delete_Point_Num);     
        t1 = chrono::high_resolution_clock::now();
        scapegoat_kd_tree.Delete_Points(cloud_decrement, Delete_Point_Num);
        t2 = chrono::high_resolution_clock::now();
        duration = chrono::duration_cast<chrono::microseconds>(t2-t1).count();
        total_duration += duration;
        printf("Delete point time cost is %0.3f ms\n",float(duration)/1e3);      
        // Search Operation  
        duration = duration - duration;               
        for (int k=0;k<Search_Time;k++){
            vector<PointType> ().swap(search_result);             
            target = generate_target_point();    
            t1 = chrono::high_resolution_clock::now();
            scapegoat_kd_tree.Nearest_Search(target, Nearest_Num, search_result);
            t2 = chrono::high_resolution_clock::now();
            duration += chrono::duration_cast<chrono::microseconds>(t2-t1).count();
        }
        printf("Search nearest point time cost is %0.3f ms\n",float(duration)/1e3);
        total_duration += duration;
        printf("Total time is %0.3f ms\n",total_duration/1e3);
        raw_cmp(target, Nearest_Num);    
        flag = cmp_point_vec(search_result, raw_cmp_result);      
        counter += 1;    
    }
    // printf("Point Cloud Points:\n");
    // printf("Target Point is : (%0.3f, %0.3f, %0.3f)\n", target.x, target.y, target.z);
    FILE *fp;
    if (!flag ){
        printf("Find Dataset for debug!\n");
        fp = freopen("Data_for_fault.txt","w",stdout);
        for (int i=0;i<point_cloud.size();i++){
            fprintf(fp,"%0.6f,%0.6f,%0.6f\n",point_cloud[i].x,point_cloud[i].y,point_cloud[i].z);
        }
        printf("Raw cmp:\n");
        print_point_vec(raw_cmp_result);
        printf("k d tree\n");
        print_point_vec(search_result);
        printf("Points in kd_tree\n");
        vector<PointType> ().swap(scapegoat_kd_tree.PCL_Storage);
        scapegoat_kd_tree.traverse_for_rebuild(scapegoat_kd_tree.Root_Node);
        print_point_vec(scapegoat_kd_tree.PCL_Storage);
        printf("Points deleted");
        print_point_vec(scapegoat_kd_tree.Points_deleted);        
        fclose(stdout);        
    } else {
        printf("Finished %d times test\n",counter);
    }
    return 0;
}

