//
//============================================================================
// Name        : Main-ILS.c
// Author      : Nicholas Richers
// E-mail      : nicholasrichers@gmail.com
// Institution : Federal University of Rio de Janeiro
//  Created by Nicholas Richers on 11/19/17.
//============================================================================


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define MAX 128


////////////////////////Choose path to read and write your instances:///////////////////////////////////////////////////////
char file_name_read[MAX]="/Users/nicholasrichers/Dropbox/UFRJ/MHOC/Trabalho/SPP-ILS-vfinal-Large/SPP-ILS-vfinal-Large/";    //
char file_name_write[MAX]="/Users/nicholasrichers/Dropbox/UFRJ/MHOC/Trabalho/SPP-ILS-vfinal-Large/SPP-ILS-vfinal-Large/results/";//
char file_list[MAX] = "Instances_list.txt";                                                                               //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int Panel_Configuration=0;
char instance_list[MAX];
bool cannot_read_file=false;
bool reset_lottery_bool=false;
bool reset_instance=false;
int reset_count=0;
bool check_solution=true;

typedef struct Criteria Criteria;

struct Criteria
{
    int iterations_Optimal_zone;
    int global_iterations;
    int global_Max_Iter;
    int global_Max_Iter2;
    int Quit_operarions;
    int reach_zone;
    float acceptance_zone;
    float acceptance_zone_relaxed;
    float acceptance_zone_relaxed2;
    int Max_Iter;
    int best_solution_improvement;
    int Optimal_solution_improvement;
    float calibration;
    float calibration_buffer;
    float Lottery_Bounds;
    float GlobalOptimal_value;
    double cpu_time_used;
    double global_cpu_time_used;
    float Optimal_percentage;
    
};

typedef struct Set Set;

struct Set
{
    int nrows;              //first line, first element
    int ncols;              //first line, second element
    int* subsetsCost;       //following lines, first element
    int* nrows_covered;     //folowing lines, second element
    int** subsets;          //folowing lines, following elements [also includes cost and nrows]
    int* index_frequency;  //How many times index appear
    int* survived;          //column elimination
    int nsurvived;          //column elimination count
    int* rank_frequency;    //rank rows index frequency
    
};

typedef struct Solution Solution;
struct Solution
{
    int SolutionCost;
    int SolutionCost_prev;
    int SolutionSize;
    int elem_changed;
    int elem_blocked;
    int* subSets;
    int* list_elements_covered;
    int* SolutionIndex;
    int* lottery_changed;
    int* lottery_blocked;
    int optimal_iteration;
    
};

Criteria* criteria;
Set* set;
Set* miniSet;
Set* nanoSet;
Solution* solution;
Solution* miniSolution;
Solution* bestSolution;
Solution* OptimalSolution;

//(1st block)
void Control_Panel(void);
void createCriteriaStruct(void);
void createSetStruct(void);
void createSolutionStruct(void);

//(2nd block)
void copySetToMini(void);
void copySetToNano(void);
void copyMiniToNano(void);
void copySolutionTominiSolution(void);
void copyminiSolution_To_Solution(void);
void copyBestSolution_To_Solution(void);
void copyOptimalSolution_To_Solution(void);
void copySolutionToBest(void);
void copyBestSolutionToOptimal(void);
void copyBestTosolution(void);

//(3rd block)
bool Get_File(FILE *ff, char f_row[]);
bool Get_Line(FILE *f, char f_row[]);
void read_file(void);

//(4th block)
int sort_index_frequency(Set *set, int try_index);
void sortSubsets_cost(Set *set);

//(5th block)
void add_subset(Set *nanoSet,int subSetIndex);
void remove_subset(Set *miniSet,int subSetIndex);
bool check_subset(int subSetIndex);
int subset_frequency(Set* nanoSet, int index);
int find_subset_index(Set* nanoSet, int index, int try);
void delete_set(void);

//(6th block)
bool checkSolution(Solution* solution);
void delete_solution(void);
void greedy_solution(void);
void matrix_reduction(void);
void greedy_solution_original(void);

//(7th block)
void Column_elimination(Set *set);
void Column_elimination_mini(Set *miniSet);
void Column_elimination_nano(Set *nanoSet);
void subsets_lottery(float cali);
int reset_lottery(void);
void local_search(void);

//(8th block)
void disturbance(void);
bool acceptance_criteria(void);
void press_results(void);
void delete_structs(void);


//main
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(void) {
    clock_t start_global, end_global;
    start_global = clock();
    FILE *List_Instances;
    double global_cpu_time_used;
    char f_row[1024];
    char read[1024];
    bool Not_EOF;

    criteria = (Criteria *) malloc(sizeof(Criteria));
    createCriteriaStruct();
    strcpy(read, file_name_read);
    strcat(read, file_list);
    List_Instances = fopen(read, "r");
    

    if(List_Instances == NULL) {
        printf("Could not open file!");
        exit(1);
    }
    
    cannot_read_file=false;

    Not_EOF=Get_File(List_Instances, f_row);
    while(Not_EOF==true) {
        clock_t start, end;
        start = clock();
        read_file();
        Control_Panel();
        
   
        //ILS Begins for selected instance
    ///////////////////////////////////////////////////////////////////////////
        if(cannot_read_file==false){
            greedy_solution();
            local_search();
            
            ///Iteration step to ILS
            while (acceptance_criteria()==false) {
                disturbance();
                local_search();
                if(cannot_read_file==true)break;
            }
        //////////////////////////////////////////////////////////////////////////////
            //End Of ILS for Current Instance
            

            end = clock();
            criteria->cpu_time_used=((double) (end - start)) / CLOCKS_PER_SEC;
            press_results();
            delete_structs();
            Not_EOF=Get_File(List_Instances, f_row);
        }cannot_read_file=false;
    }

    end_global = clock();
    global_cpu_time_used=((double) (end_global - start_global)) / CLOCKS_PER_SEC;
    printf("cpu_time_used (s): %f\n", global_cpu_time_used);
    return 0;
}




//(1st block)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


void Control_Panel(void){
    
    
    ///best configuraration to mini instances
    if (Panel_Configuration==1){
        criteria->acceptance_zone=0.99;                 //max=.99999
        criteria->acceptance_zone_relaxed=0.95;         //max=.99999
        criteria->acceptance_zone_relaxed2=0.9;         //max=.99999
        criteria->reach_zone=1;                         //if==2;jumps straight to zone_relaxed
        criteria->calibration_buffer=0.3;               ///min = 0 less disturbance||max = 1 more disturbance
        criteria->Lottery_Bounds=0.45;                  //max=.499999
        criteria->global_Max_Iter=.5*(1000000);         //.5M
        criteria->global_Max_Iter2=.75*(1000000);       //.75M
        criteria->Quit_operarions=1*(1000000);          //1M
    }
    
    ///best configuraration to small instances
    if (Panel_Configuration==2){
        criteria->acceptance_zone=0.99;                 //max=.99999
        criteria->acceptance_zone_relaxed=0.95;         //max=.99999
        criteria->acceptance_zone_relaxed2=0.9;         //max=.99999
        criteria->reach_zone=1;                         //if==2;jumps straight to zone_relaxed
        criteria->calibration_buffer=0.8;               ///min = 0 less disturbance||max = 1 more disturbance
        criteria->Lottery_Bounds=0.4;                  //max=.499999
        criteria->global_Max_Iter=.5*(1000000);         //0.5M
        criteria->global_Max_Iter2=.75*(1000000);       //0.75M
        criteria->Quit_operarions=1*(1000000);          //1M
    }
    
    
    
    ///best configuraration to medium instances
    if (Panel_Configuration==3){
        criteria->acceptance_zone=0.99;                 //max=.99999
        criteria->acceptance_zone_relaxed=0.95;         //max=.99999
        criteria->acceptance_zone_relaxed2=0.9;         //max=.99999
        criteria->reach_zone=1;                         //if==2;jumps straight to zone_relaxed
        criteria->calibration_buffer=0.8;               ///min = 0 less disturbance||max = 1 more disturbance
        criteria->Lottery_Bounds=0.3;                  //max=.499999
        criteria->global_Max_Iter=0.1*(1000000);         //1
        criteria->global_Max_Iter2=0.2*(1000000);       //1.5M
        criteria->Quit_operarions=0.3*(1000000);          //1M
    }
    
    
    ///best configuraration to large instances
    if (Panel_Configuration==4){
        criteria->acceptance_zone=0.97;                 //max=.99999
        criteria->acceptance_zone_relaxed=0.9;         //max=.99999
        criteria->acceptance_zone_relaxed2=0.8;         //max=.99999
        criteria->reach_zone=1;                         //if==2;jumps straight to zone_relaxed
        criteria->calibration_buffer=0.8;               ///min = 0 less disturbance||max = 1 more disturbance
        criteria->Lottery_Bounds=0.3;                  //max=.499999
        criteria->global_Max_Iter=0.3*(10000);         //100k
        criteria->global_Max_Iter2=0.5*(10000);       //200k
        criteria->Quit_operarions=0.7*(10000);        //300k
    }
    
    
    ///best configuraration to toy instances
    if (Panel_Configuration==5){
        criteria->acceptance_zone=0.99;                 //max=.99999
        criteria->acceptance_zone_relaxed=0.95;         //max=.99999
        criteria->acceptance_zone_relaxed2=0.9;         //max=.99999
        criteria->reach_zone=1;                         //if==2;jumps straight to zone_relaxed
        criteria->calibration_buffer=0.8;               ///min = 0 less disturbance||max = 1 more disturbance
        criteria->Lottery_Bounds=0.4;                  //max=.499999
        criteria->global_Max_Iter=1*(1000000);         //1M
        criteria->global_Max_Iter2=1.5*(1000000);       //1.5M
        criteria->Quit_operarions=3*(1000000);          //3M
    }
    
    
    
}


void createCriteriaStruct(void){
    criteria->iterations_Optimal_zone=0;
    criteria->global_iterations=1;
    Panel_Configuration=0;
}

void createSetStruct(void){
    
    set->subsetsCost= (int *) malloc(sizeof(int) *  set->ncols);
    set->nrows_covered = (int *) malloc(sizeof(int) * set->ncols);
    set->subsets = (int**) malloc(sizeof(int*) * set->ncols);
    set->index_frequency= (int *) malloc(sizeof(int) *  set->nrows+1);
    set->survived = (int *) malloc(sizeof(int) *  set->ncols);
    set->rank_frequency=(int *) malloc(sizeof(int) *  set->nrows+1);
    
    miniSet->subsetsCost= (int *) malloc(sizeof(int) *  set->ncols);
    miniSet->nrows_covered = (int *) malloc(sizeof(int) * set->ncols);
    miniSet->subsets = (int **) malloc(sizeof(int *) * set->ncols);
    miniSet->index_frequency= (int *) malloc(sizeof(int) *  set->nrows+1);
    miniSet->survived = (int *) malloc(sizeof(int) *  set->ncols);
    miniSet->rank_frequency=(int *) malloc(sizeof(int) *  set->nrows+1);
    
    nanoSet->subsetsCost= (int *) malloc(sizeof(int) *  set->ncols);
    nanoSet->nrows_covered = (int *) malloc(sizeof(int) * set->ncols);
    nanoSet->subsets = (int **) malloc(sizeof(int *) * set->ncols);
    nanoSet->index_frequency= (int *) malloc(sizeof(int) *  set->nrows+1);
    nanoSet->survived = (int *) malloc(sizeof(int) *  set->ncols);
    nanoSet->rank_frequency=(int *) malloc(sizeof(int) *  set->nrows+1);
    
    int index;
    for(index=0; index<=set->nrows; index++){
        set->index_frequency[index]=0;
        miniSet->index_frequency[index]=0;
        nanoSet->index_frequency[index]=0;
        set->rank_frequency[index]=0;
        miniSet->rank_frequency[index]=0;
        nanoSet->rank_frequency[index]=0;
    }
    set->index_frequency[0]=1000000;
    miniSet->index_frequency[0]=1000000;
    nanoSet->index_frequency[0]=1000000;
    set->rank_frequency[0]=0;
    miniSet->rank_frequency[0]=0;
    nanoSet->rank_frequency[0]=0;
}


void createSolutionStruct() {
    int i,j;
    
    solution = (Solution *) malloc(sizeof(Solution));
    miniSolution = (Solution *) malloc(sizeof(Solution));
    bestSolution = (Solution *) malloc(sizeof(Solution));
    OptimalSolution = (Solution *) malloc(sizeof(Solution));
    
    solution->subSets = (int*) malloc(sizeof(int) * (set->ncols));
    miniSolution->subSets = (int*) malloc(sizeof(int) * (set->ncols));
    bestSolution->subSets = (int*) malloc(sizeof(int) * (set->ncols));
    OptimalSolution->subSets = (int*) malloc(sizeof(int) * (set->ncols));
    
    
    solution->list_elements_covered = (int*) malloc(sizeof(int) * (set->nrows+1));
    miniSolution->list_elements_covered = (int*) malloc(sizeof(int) * (set->nrows+1));
    bestSolution->list_elements_covered = (int*) malloc(sizeof(int) * (set->nrows+1));
    OptimalSolution->list_elements_covered = (int*) malloc(sizeof(int) * (set->nrows+1));
    for(i=0; i<(set->ncols); i++){
        solution->subSets[i] = 0;
        miniSolution->subSets[i] = 0;
        bestSolution->subSets[i] = 0;
        OptimalSolution->subSets[i] = 0;
        
        miniSolution->SolutionSize += set->subsets[i][0];
        bestSolution->SolutionSize += set->subsets[i][0];
        OptimalSolution->SolutionSize+=  set->subsets[i][0];
    }
    solution->SolutionSize=0; //here is zero!
    solution->SolutionCost=0;
    miniSolution->SolutionCost = 0;
    bestSolution->SolutionCost = 0;
    OptimalSolution->SolutionCost = 0;
    OptimalSolution->SolutionCost_prev = 0;
    
    for(j=0; j<=(set->nrows); j++){
        solution->list_elements_covered[j] = 0;
        miniSolution->list_elements_covered[j] = 0;
    }
    solution->list_elements_covered[0]=1;
    miniSolution->list_elements_covered[0]=1;
}

//(2nd block)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


void copySetToMini(void){
    int i,k;
    miniSet->nrows = set->nrows;
    miniSet->ncols = set->ncols;
    miniSet->nsurvived = set->nsurvived;
    
    for(i=0; i<miniSet->ncols; i++){
        miniSet->survived[i]= set->survived[i];
        miniSet->subsetsCost[i]=set->subsetsCost[i];
        miniSet->nrows_covered[i]= set->nrows_covered[i];
            }
    
    for(k=1;k<=miniSet->nrows;k++){
        miniSet->index_frequency[k]=set->index_frequency[k];
        miniSet->rank_frequency[k]=set->rank_frequency[k];
        
        
    }
}


void copySetToNano(void){
    int i,k;
    nanoSet->nrows = set->nrows;
    nanoSet->ncols = set->ncols;
    nanoSet->nsurvived = set->nsurvived;
    
    for(i=0; i<nanoSet->ncols; i++){
        nanoSet->survived[i]= set->survived[i];
        nanoSet->subsetsCost[i]=set->subsetsCost[i];
        nanoSet->nrows_covered[i]= set->nrows_covered[i];
        }
    for(k=1;k<=nanoSet->nrows;k++){
        nanoSet->index_frequency[k]=set->index_frequency[k];
        nanoSet->rank_frequency[k]=set->rank_frequency[k];
        
    }}




void copyMiniToNano(void){
    int i,j,k;
    nanoSet->nrows = miniSet->nrows;
    nanoSet->ncols = miniSet->ncols;
    nanoSet->nsurvived = miniSet->nsurvived;
    
    for(i=0; i<nanoSet->ncols; i++){
        nanoSet->survived[i]= miniSet->survived[i];
        nanoSet->subsetsCost[i]=miniSet->subsetsCost[i];
        nanoSet->nrows_covered[i]= miniSet->nrows_covered[i];
        for(j=0; j<(nanoSet->nrows_covered[i]+2); j++){
            nanoSet->subsets[i][j] = miniSet->subsets[i][j];
        }}
    for(k=1;k<=nanoSet->nrows;k++){
        nanoSet->index_frequency[k]=miniSet->index_frequency[k];
        nanoSet->rank_frequency[k]=miniSet->rank_frequency[k];
    }}




void copySolutionTominiSolution() {
    int i,k,j=0;
    miniSolution->SolutionCost = solution->SolutionCost;
    miniSolution->SolutionSize = solution->SolutionSize;
    //free(miniSolution->SolutionIndex);
    miniSolution->SolutionIndex =(int*) malloc(sizeof(int) * (miniSolution->SolutionSize));
    for(i=0;i<set->ncols;i++){
        miniSolution->subSets[i] = solution->subSets[i];
        if(miniSolution->subSets[i]==1){
            miniSolution->SolutionIndex[j]=i;
            j++;
        }
    }
    //printf("\nsolution to miniSolution:");
    for(k=0;k<=set->nrows;k++)
        miniSolution->list_elements_covered[k]=solution->list_elements_covered[k];
    //printf("-%d", miniSolution->list_elements_covered[k]);}
}




void copyminiSolution_To_Solution(void){
    int i,k,j=0;
    solution->SolutionCost = miniSolution->SolutionCost;
    solution->SolutionSize = miniSolution->SolutionSize;
    //free(solution->SolutionIndex);
    solution->SolutionIndex =(int*) malloc(sizeof(int) * (solution->SolutionSize));
    for(i=0;i<set->ncols;i++){
        solution->subSets[i] = miniSolution->subSets[i];
        if(solution->subSets[i]==1){
            solution->SolutionIndex[j]=i;
            j++;
        }
    }//printf("\nmini to solution:");
    for(k=0;k<=set->nrows;k++)
        solution->list_elements_covered[k]=miniSolution->list_elements_covered[k];
    // printf("-%d-", solution->list_elements_covered[k]);}
}



void copyBestSolution_To_Solution(void){
    int i,k,j=0;
    solution->SolutionCost = bestSolution->SolutionCost;
    solution->SolutionSize = bestSolution->SolutionSize;
    //free(miniSolution->SolutionIndex);
    solution->SolutionIndex =(int*) malloc(sizeof(int) * (solution->SolutionSize));
    for(i=0;i<set->ncols;i++){
        solution->subSets[i] = bestSolution->subSets[i];
        if(solution->subSets[i]==1){
            solution->SolutionIndex[j]=i;
            j++;
        }
    }//printf("\nmini to solution:");
    for(k=0;k<=set->nrows;k++)
        solution->list_elements_covered[k]=bestSolution->list_elements_covered[k];
    // printf("-%d-", solution->list_elements_covered[k]);}
}


void copyOptimalSolution_To_Solution(void){
    int i,k,j=0;
    solution->SolutionCost = OptimalSolution->SolutionCost;
    solution->SolutionSize = OptimalSolution->SolutionSize;
    //free(solution->SolutionIndex);
    solution->SolutionIndex =(int*) malloc(sizeof(int) * (solution->SolutionSize));
    for(i=0;i<set->ncols;i++){
        solution->subSets[i] = OptimalSolution->subSets[i];
        if(solution->subSets[i]==1){
            solution->SolutionIndex[j]=i;
            j++;
        }
    }//printf("\nmini to solution:");
    for(k=0;k<=set->nrows;k++)
        solution->list_elements_covered[k]=OptimalSolution->list_elements_covered[k];
    // printf("-%d-", solution->list_elements_covered[k]);}
}


void copySolutionToBest() {
    int i,j=0;
    criteria->best_solution_improvement++;
    bestSolution->SolutionCost = solution->SolutionCost;
    bestSolution->SolutionSize = solution->SolutionSize;
    //printf("\ncost: %d:\n", bestSolution->SolutionCost);
     //free(bestSolution->SolutionIndex);
    bestSolution->SolutionIndex =(int*) malloc(sizeof(int) * (bestSolution->SolutionSize));
    for(i=0;i<set->ncols;i++){
        bestSolution->subSets[i] = solution->subSets[i];
        if(bestSolution->subSets[i]==1){
            bestSolution->SolutionIndex[j]=i;
            j++;
        }
    }
}

void copyBestSolutionToOptimal() {
    int i,j=0;
    reset_count=0;
    criteria->Optimal_solution_improvement++;
    OptimalSolution->SolutionCost = bestSolution->SolutionCost;
    criteria->Optimal_percentage=(criteria->GlobalOptimal_value/OptimalSolution->SolutionCost);
    if( OptimalSolution->SolutionCost>0){
        printf("Cost: %d ",OptimalSolution->SolutionCost);
        printf("- %.2f%% ", (criteria->Optimal_percentage*100));
        printf("Iterations: %d\n", criteria->global_iterations);
        press_results();
    }
    OptimalSolution->optimal_iteration=criteria->global_iterations;
    OptimalSolution->SolutionSize = bestSolution->SolutionSize;
    OptimalSolution->SolutionIndex =(int *) malloc(sizeof(int) * (OptimalSolution->SolutionSize));
    for(i=0;i<set->ncols;i++){
        OptimalSolution->subSets[i] = bestSolution->subSets[i];
        if(OptimalSolution->subSets[i]==1){
            OptimalSolution->SolutionIndex[j]=i;
            j++;
        }
    }
}


void copyBestTosolution(void){
    int i,j=0;
    
    solution->SolutionCost = bestSolution->SolutionCost;
    solution->SolutionSize = bestSolution->SolutionSize;
     //free(solution->SolutionIndex);
    solution->SolutionIndex =(int *) malloc(sizeof(int) * (solution->SolutionSize));
    for(i=0;i<set->ncols;i++){
        solution->subSets[i] = bestSolution->subSets[i];
        if(solution->subSets[i]==1){
            solution->SolutionIndex[j]=i;
            j++;
        }
    }
    
}


//(3rd block)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Get_File(FILE *ff, char f_row[]) {
    
    char *p;
    bool not_eof = false;
    char *elem_file;
    char instance_name[MAX]="SPP";
    
    strcpy(file_name_write,file_name_read);
    strcat(file_name_write, "Results:_");
    
    while (fgets(f_row, 1024, ff) != NULL) {
        p=strchr(f_row, (int) '\n');
        elem_file = strtok(f_row, " ");
        strcat(instance_name, elem_file);
        strcat(instance_name, ".txt");
        
        strcpy(instance_list, instance_name);
        elem_file = strtok(NULL, " ");
        if(elem_file==NULL){
            printf("End of Instances List");
            exit(1);
        }
        criteria->GlobalOptimal_value=atoi(elem_file);
        
        if ( p != NULL )
            *p = '\0';
        if (*f_row != '\0') {
            not_eof = true;
            break;
        }
    }
    return (not_eof);
    
}


bool Get_Line(FILE *f, char f_row[]) {
    /* Read a line of possibly commented input from the file *f.*/
    char *p;
    bool not_eof = false;
    while ( fgets(f_row, 1024, f) != NULL) {
        p=strchr(f_row, (int) '\n');
        if ( p != NULL )
            *p = '\0';
        if (*f_row != '\0') {
            not_eof = true;
            break;
        }
    }
    return (not_eof);
}



void read_file(void) {
    FILE *Current_Instance;
    int j=0, lin_file=0, i=0, index;
    int subSetSize=0;
    int nCurrSubSet=0;
    char * element_file;
    char f_row[1024];
    char file_row[1024];
    char read[MAX];
    char file[MAX];
    set = (Set *) malloc(sizeof(Set));
    miniSet = (Set *) malloc(sizeof(Set));
    nanoSet = (Set *) malloc(sizeof(Set));
    
    

    strcpy(read,file_name_read);
    strcpy(file,instance_list);
    strcat(read, file);
    Current_Instance = fopen(read, "r");
    printf("File:%s\n", instance_list);
    printf("Global Optimal Cost: %f\n", criteria->GlobalOptimal_value);
    
    
    if(Current_Instance == NULL) {
        printf("Could not open file - check line 20!");
        cannot_read_file=true;
    }
    if(cannot_read_file==false){
        while(Get_Line(Current_Instance, file_row)) {
            
            if(lin_file==0) { //read first line of file
                
                element_file = strtok(file_row, " ");
                while (element_file != NULL) {
                    if(j==0){
                        set->nrows = atoi(element_file);  //numero de linhas ok
                    }
                    if(j==1){
                        set->ncols=atoi(element_file);  //numero de colunas ok
                    }
                    j++;
                    element_file = strtok(NULL, " ");  // quebra a string ate o espaco e transforma o anterior em NULL;
                }
                printf("Instance Size: Col:%d x Rows:%d\n",set->ncols, set->nrows);
                if(set->ncols>100&&set->ncols<=700)Panel_Configuration=1;
                if(set->ncols>700&&set->ncols<=3200)Panel_Configuration=2;
                if(set->ncols>3200&&set->ncols<=20000)Panel_Configuration=3;
                if(set->ncols>20000)Panel_Configuration=4;
                if(set->ncols<=100)Panel_Configuration=5;
                printf("Panel Configuration: %d\n", Panel_Configuration);
                
                createSetStruct();
                
            }
            else{
                strcpy(f_row, strchr(file_row, (int) ' '));
                element_file = strtok(f_row, " ");
                subSetSize=atoi(element_file);
                element_file = strtok(file_row, " ");
                set->subsets[nCurrSubSet] = (int *) malloc(sizeof(int) * (subSetSize+2));
                //miniSet->subsets[nCurrSubSet] = (int *) malloc(sizeof(int) * (subSetSize+2));
                //nanoSet->subsets[nCurrSubSet] = (int *) malloc(sizeof(int) * (subSetSize+2));
                //set->nrows_covered[nCurrSubSet] = subSetSize;
                set->subsetsCost[nCurrSubSet] = atoi(file_row);
                set->survived[nCurrSubSet] = 1;
                
                
                while (element_file != NULL) {
                    set->subsets[nCurrSubSet][i] = atoi(element_file);
                    index=atoi(element_file);
                    
                    if(i==1){
                        set->nrows_covered[nCurrSubSet] =  set->subsets[nCurrSubSet][i];
                    }
                    
                    if(i>1){
                        set->index_frequency[index]++;
                        miniSet->index_frequency[index]++;
                        nanoSet->index_frequency[index]++;
                    }
                    
                    element_file = strtok(NULL, " ");
                    i++;
                }
                nCurrSubSet++;
                i=0;
            }
            lin_file++;
        }fclose(Current_Instance);
    }
    
    
    for(i=0;i<set->ncols; i++){

    }
    
    
}


//(4th block)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////



int sort_index_frequency(Set *nanoSet, int try_index){

    int ind=0;
    int j=0;
    int k,i;
    
    // which rows are already covered
    for(k=0;k<=nanoSet->nrows;k++){
        nanoSet->rank_frequency[k]=0;
        // printf("[%d_%d] ",k, solution->list_elements_covered[k]);
        if(solution->list_elements_covered[k]==1){
            nanoSet->index_frequency[k]=1000000+k; //if the element is already covered;
             //printf("(%d_%d) ",k,  nanoSet->index_frequency[k]);
            
        }}
    
    
    // do the ranking
    for(i=1; i<=nanoSet->nrows; i++){
        for(j=1; j<=nanoSet->nrows; j++){
            if(nanoSet->index_frequency[i] > nanoSet->index_frequency[j]){
                nanoSet->rank_frequency[i]=nanoSet->rank_frequency[i]+1;
                
            }}
        nanoSet->rank_frequency[i]++;
    }
    
    //avoiding draws
    for(i=1; i<=nanoSet->nrows; i++){
        for(j=2; j<=nanoSet->nrows; j++){
            if( nanoSet->rank_frequency[i] ==   nanoSet->rank_frequency[j] && i!=j){
                nanoSet->rank_frequency[j]++;;
                
            }}}
    
    
    
    for(i=1; i<=nanoSet->nrows; i++){
        if(nanoSet->rank_frequency[i]==try_index){
            ind = i;
        }
    }
    
    


    
    
    /////////////////////
    //check - error
    if(try_index>(set->nrows/4)){
        if(cannot_read_file==false){
        ind=reset_lottery();
        }
        //check
        /*
         printf("\n**Error: Lottery Bounds too low for this instance**\n\n");
         
         printf("\n******\n try %d`th index: [%d] ",try_index, ind);
         printf("\nRANK INDEX:\n");
         for(j=1; j<=nanoSet->nrows; j++){
         printf("%d] ", nanoSet->index_frequency[j]);
         printf(" [%d] ", solution->list_elements_covered[j]);
         printf("    || [%d] ", nanoSet->rank_frequency[j]);
         printf("\n");
         }*/
        
    }
    
    return ind;
}




//This sorts the subsets by cost
void sortSubsets_cost(Set *set) {
    
    int i,j;
    int size_temp;
    int cost_temp;
    int *vetor_temp=NULL;
    
    
    for(i=set->ncols-1; i>=1;i--){
        for(j=0; j<i; j++){
            if(set->subsets[j][0] > set->subsets[j+1][0]) {
                
                //sortsubsets by cost
                vetor_temp = set->subsets[j];
                set->subsets[j]= set->subsets[j+1];
                set->subsets[j+1]= vetor_temp;
                
                //adjusts size of vector
                size_temp = set->nrows_covered[j];
                set->nrows_covered[j]=set->nrows_covered[j+1];
                set->nrows_covered[j+1]=size_temp;
                
                //adjusts subset cost
                cost_temp = set->subsetsCost[j];
                set->subsetsCost[j]=set->subsetsCost[j+1];
                set->subsetsCost[j+1]=cost_temp;
                
            }}}
    
    
    

    //check
    /*
     int h, l;
     printf("\n\nSorted:\n");
     for(h=0; h<set->ncols; h++){
     printf("(%d)", set->subsets[h][0]);
     printf("[%d] ", set->subsets[h][1]);
     for(l=1; l<=set->nrows_covered[h]; l++){
     printf("%d ", set->subsets[h][l+1]);
     }
     printf("\n");
     
     }
     */
    
}


//(5th block)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void add_subset(Set* nanoSet, int subSetIndex){
    int j, index;
    
    solution->SolutionCost += nanoSet->subsetsCost[subSetIndex];
    solution->SolutionSize++;
    solution->subSets[subSetIndex]=1;
    for(j=0; j<set->subsets[subSetIndex][1]; j++){
         //printf("{%d} ", set->subsets[subSetIndex][j+2]);
        index=set->subsets[subSetIndex][j+2];
        solution->list_elements_covered[index]=1;
        }
}

void remove_subset(Set *miniSet, int subSetIndex){
    int j;
    solution->SolutionCost -= miniSet->subsetsCost[subSetIndex];
    solution->SolutionSize--;
    solution->subSets[subSetIndex]=0;
    for(j=0; j<set->nrows_covered[subSetIndex]; j++){
        solution->list_elements_covered[miniSet->subsets[subSetIndex][j+2]]=0;
    }
}


bool check_subset(int subSetIndex){
    int j;
    for(j=0; j<set->subsets[subSetIndex][1]; j++){
        //printf("%d|", set->subsets[subSetIndex][j+2]);
        // int k; for(k=0;k<=set->nrows;k++){printf("{{%d}}", solution->list_elements_covered[k]);} //check covered elments
        if(solution->list_elements_covered[set->subsets[subSetIndex][j+2]]==1){
            return false; //it's not a viable solution
        }
    }
    return true;
}



int subset_frequency(Set* nanoset, int index){
    int candidate,j,frequency=0;
    for(candidate=0; candidate<nanoSet->ncols; candidate++){
        if(nanoSet->survived[candidate]==1){
            for(j=0; j<set->subsets[index][1]; j++){
                if(nanoSet->subsets[candidate][j+2]==index){
                    frequency++;
                }}}}
    
    return frequency;
}




int find_subset_index(Set* nanoSet, int index, int try){
    int candidate,j,temp=0;
try_again:
    for(candidate=0; candidate<nanoSet->ncols; candidate++){
        if(nanoSet->survived[candidate]==1){
            for(j=0; j<set->subsets[candidate][1]; j++){
                if(nanoSet->subsets[candidate][j+2]==index){
                    temp++;
                    if(temp==try)
                        return candidate;
                }}}}
    
    if(candidate==set->ncols){
        reset_lottery_bool=true;
    }
    
    return candidate-1;
    
}



void delete_set(void){
    
    int i,j,k;
    set->nrows=0;
    set->ncols=0;
    set->nsurvived=0;
    
    for(i=0; i<set->ncols; i++){
        set->survived[i]=0;
        set->subsetsCost[i]=0;
        set->nrows_covered[i]=0;
        for(j=0; j<(set->nrows_covered[i]+2); j++){
            set->subsets[i][j]=0;
        }}
    
    for(k=1;k<=set->nrows;k++)
        set->index_frequency[k]=0;
    set->rank_frequency[k]=0;
    
}





//(6th block)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


bool checkSolution(Solution* solution) {
    
    int i,k;
    int j=0;
    for(i=1; i<=set->nrows; i++) {
        if(solution->list_elements_covered[i] == 0)
            return false;
    }
    
    solution->SolutionIndex =(int *) malloc(sizeof(int) * (solution->SolutionSize));
    for(k=0; k<(set->ncols); k++){
        if(solution->subSets[k]==1){
            solution->SolutionIndex[j]=k;
            j++;
        }
    }
    
    return true;
}


void delete_solution(void){
    int j,k;
    solution->SolutionCost=0;
    solution->SolutionSize=0;
    for(j=1; j<=(set->nrows); j++){
        solution->list_elements_covered[j] = 0;
    }
    for(k=0; k<(set->ncols); k++){
        solution->subSets[k] = 0;
    }
}



void greedy_solution(void) {
    int i=0, try=1,candidate=0;
    int index=0, try_index=0;
    bool new_try=false;
    int j;

    createSolutionStruct();
    //matrix_reduction();
    sortSubsets_cost(set); //sortsubset by cost
    
    
    for(i=0; i<set->ncols; i++){
        miniSet->subsets[i] = (int *) malloc(sizeof(int) * (set->nrows_covered[i]+2));
        nanoSet->subsets[i] = (int *) malloc(sizeof(int) * (set->nrows_covered[i]+2));
        for(j=0; j<(set->nrows_covered[i]+2);j++){
            miniSet->subsets[i][j]=set->subsets[i][j];
            nanoSet->subsets[i][j]=set->subsets[i][j];
            
            
        }}

    copySetToMini();
    copySetToNano();
    

    
try_again:
    
    //algorithm begins
    while(checkSolution(solution)==false){

      
        if(reset_count>5){
            reset_count=0;
            delete_solution();
            greedy_solution_original();
            break;
        }
            
            
            
            
        if((nanoSet->nsurvived==0 || reset_lottery_bool==true)  && checkSolution(solution)==false){
            reset_lottery_bool=false;  //printf("\n\n *******RESTART******* \n");
            delete_solution();
            new_try=true;
            //matrix_reduction();
            copySetToNano();
            try_index++;
            index=sort_index_frequency(nanoSet, try_index);
            i=0;
            if(try_index>=nanoSet->nrows){
                i=nanoSet->ncols;  //Max try reached, try again
            }}
        
        if (new_try==false){
            index=sort_index_frequency(nanoSet, 1);
        }
        
        
        new_try=false;
        candidate=find_subset_index(nanoSet, index,1);
        add_subset(nanoSet,candidate);                        //printf("subset added(%d)\n", candidate);
        Column_elimination_nano(nanoSet);                 //for(k=1; k<=nanoSet->nrows; k++){ printf("[%d_%d]\n",k, solution->list_elements_covered[k]);}
                                                          //  printf("survived: %d\n", nanoSet->nsurvived);
        
        
        
        i++;
        //if cannot find a feasible solution, try again
        if(i>=(nanoSet->ncols-1)&& checkSolution(solution)==false){
            try_index=1;
            i=try;
            try++;
            delete_solution();
            //printf("\nTry number %d: FAILED", (try));
            if(try>=nanoSet->ncols){
                printf("No Feasible Solution\n");
                printf("\n\n***********************************\n\n\n");
                FILE *Instances;
                char write[MAX];
                char file[MAX];
                strcpy(file, instance_list);
                strcpy(write, file_name_write);
                strcat(write, file);
                Instances = fopen(write, "w");
                fprintf(Instances, "Instance_Name: %s\n", instance_list);
                fprintf(Instances, "Instance_Size: %dx%d\n", nanoSet->ncols,nanoSet->nrows);
                fprintf(Instances, "No Feasible Solution\n");
                delete_structs();
                cannot_read_file=true; break;
            }
            add_subset(nanoSet,i);
            // printf("/////");
            // printf("%d ", i);
            i=0;
            goto try_again;
        }
        if(cannot_read_file==true) break;
    }
    if(cannot_read_file==false){
     printf("Greedy ");
    copySolutionToBest();
    copyBestSolutionToOptimal();
    }
}



void matrix_reduction(void){
    int i,j,k,candidate, index;
    bool elimination=true;
    
    
    //elimination 1
    for(k=0; k<set->ncols; k++){
        for(i=0; i<set->ncols; i++){
            if(set->nrows_covered[k]==set->nrows_covered[i]){
                for(j=2; j<set->nrows_covered[k]; j++){
                    if(set->subsets[k][j]!=set->subsets[i][j]){
                        elimination=false;
                        index=set->subsets[k][j];
                        set->index_frequency[index]++;
                        miniSet->index_frequency[index]++;
                        nanoSet->index_frequency[index]++;
                    }
                    if(elimination==true){
                        if(set->subsets[k][0]>set->subsets[i][0]){
                            set->survived[k]=0;
                        }else  set->survived[i]=0;
                    }}}}}
    
    
    //elimination 2
    
    for(j=0; (j<=set->nrows); j++){
        // printf( "index[%d] freq: %d",j, set->index_frequency[j][1]);
        if(set->index_frequency[j]==1){
            candidate=find_subset_index(set,j,1);
            add_subset(set, candidate); //printf(" column: %d", candidate);
            Column_elimination(set);
        }}
    
}



void greedy_solution_original(void) {
    int i=1;
    int try=0;
    createSolutionStruct();
    add_subset(set, 0);   //add first column to greedy solution
try_again:
    while(checkSolution(solution)==false) {
        
        //add columns to greedy solution
        if(check_subset(i)==true) {
            add_subset(set, i);
            //printf("%d ", i);
        }
        i++;
        
        //checking if greedy has a feasible solution
        if(i>=(set->ncols-1)&& checkSolution(solution)==false){
            try++;
            i=try;
            delete_solution();
            //printf("\nTry number %d: FAILED", (try));
            if(try>=set->ncols){
                printf("No Feasible Solution\n");
                printf("\n\n***********************************\n\n\n");
                FILE *Instances;
                char write[MAX];
                char file[MAX];
                strcpy(file, instance_list);
                strcpy(write, file_name_write);
                strcat(write, file);
                Instances = fopen(write, "w");
                fprintf(Instances, "Instance_Name: %s\n", instance_list);
                fprintf(Instances, "Instance_Size: %dx%d\n", set->ncols,set->nrows);
                fprintf(Instances, "No Feasible Solution\n");
                delete_structs();
                cannot_read_file=true; break;
            }
            add_subset(set,i);
            //printf("/////");
            //printf("%d ", i);
            i=0;
            goto try_again;
        }
        
    }
}



//(7th block)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void Column_elimination(Set *set){
    int i,j,index;
    
    for(i=1; i<=set->nrows; i++){
        set->index_frequency[i]=0;
    }
    
    set->nsurvived=0;
    for(i=0; i<set->ncols; i++){
        set->survived[i]=0;
        if(check_subset(i)==true){
            set->survived[i]=1;
            set->nsurvived++;
            for(j=0; j<set->nrows_covered[i];j++){
                index=set->subsets[i][j+2];
                set->index_frequency[index]++;
            }}}}



void Column_elimination_mini(Set *miniSet){
    int i,j,index;
    
    for(i=1; i<=set->nrows; i++){
        miniSet->index_frequency[i]=0;
    }
    
    miniSet->nsurvived=0;
    for(i=0; i<miniSet->ncols; i++){
        miniSet->survived[i]=0;
        
        if(check_subset(i)==true){
            miniSet->survived[i]=1;
            miniSet->nsurvived++;
            for(j=0; j<miniSet->nrows_covered[i];j++){
                index=miniSet->subsets[i][j+2];
                miniSet->index_frequency[index]++;
            }
        }
    }
    //check
    // printf(" \nsurvived: %d ", miniSet->nsurvived);
    
}

void Column_elimination_nano(Set *nanoSet){
    int i,j,index;
    for(i=1; i<=nanoSet->nrows; i++){
        nanoSet->index_frequency[i]=0;
    }
    nanoSet->nsurvived=0;
    for(i=0; i<nanoSet->ncols; i++){
        nanoSet->survived[i]=0;
        if(check_subset(i)==true){
            nanoSet->survived[i]=1;
            nanoSet->nsurvived++;
            for(j=0; j<nanoSet->nrows_covered[i];j++){
                index=nanoSet->subsets[i][j+2];
                nanoSet->index_frequency[index]+=1;
            }
        }
    }
    //check
     //printf(" survived: %d ", nanoSet->nsurvived);
}

void subsets_lottery(float cali){
    int i,j,l;
    int index=0;
    int index2=0;
    int count=0;
    float perc_change= (criteria->Lottery_Bounds)+(criteria->Lottery_Bounds*cali);
    
    solution->elem_changed = perc_change*(solution->SolutionSize);
    solution->elem_blocked = solution->SolutionSize - solution->elem_changed;
    
    
    int lottery_values[solution->elem_blocked];
    int non_lottery[solution->elem_changed];
    
    solution->lottery_changed = (int *)malloc(sizeof(int) * (solution->elem_changed));
    solution->lottery_blocked = (int *)malloc(sizeof(int) * (solution->elem_blocked));
    
    //printf("\nlottery:\n");
    srand((unsigned)time(NULL) );
new_lottery:
    lottery_values[0]=rand() % solution->SolutionSize;
    // printf("%d ", lottery_values[0]);
    for (i=1; i <solution->elem_blocked; i++){
        lottery_values[i]=rand() % solution->SolutionSize;
        //printf("%d ", lottery_values[i]);
        for (j=i-1; j>=0; j--){
            if(lottery_values[i]== lottery_values[j]){
                // printf("try again\n");
                goto new_lottery;  //avoid repeated elements
            }
        }
    }
    
    //not drawn elements
    for(l=0; l<solution->SolutionSize; l++){
        for(index=0; index<solution->elem_blocked; index++){
            if(l==lottery_values[index]){
                count++;
            }}
        if(count==0){
            non_lottery[index2]=l;
            index2++;
        }
        count=0;
    }
    
    for(index=0; index<solution->elem_blocked; index++){
        solution->lottery_blocked[index]=solution->SolutionIndex[lottery_values[index]];
    }
    
    for(index=0; index<solution->elem_changed; index++){
        solution->lottery_changed[index]=solution->SolutionIndex[non_lottery[index]];
    }
    
    //check lottery
    /*
     int h,g;
     
     printf("\ncolumn blocked\n");
     for(h=0; h< solution->elem_blocked; h++){
     printf("(%d) ", solution->lottery_blocked[h]);
     for(g=0; g<set->nrows_covered[solution->lottery_blocked[h]];g++)
     printf("%d ",set->subsets[solution->lottery_blocked[h]][g+2]);
     }
     
     printf("\ncolumn changed\n");
     for(h=0; h< solution->elem_changed; h++){
     printf("(%d) ", solution->lottery_changed[h]);
     for(g=0; g<set->nrows_covered[solution->lottery_changed[h]];g++)
     printf("%d ",set->subsets[solution->lottery_changed[h]][g+2]);
     }
     */
}

int reset_lottery(){
    int try=1,k,try_next=0, index=0;
    int remaining=10;
    reset_count++; //printf("Reset count remaining: %d\n", remaining-reset_count);
    criteria->global_iterations++;
    if(reset_count>=remaining) {
        reset_instance=true;
        cannot_read_file=true; //cancel the instance iteration
       // printf("*******RESET ACTIVATED**********\n");
    }
    //remove lottery subsets
    for(k = 0; k < solution->elem_changed; k++){
        remove_subset(miniSet, solution->lottery_changed[k]);
    }
    
    Column_elimination_mini(miniSet);
    copyBestTosolution();
    copySolutionTominiSolution();
    copyMiniToNano();
    try=1; try_next=0;
    index=sort_index_frequency(nanoSet,try_next+1);
    
    if(index==0) {
        //printf("*******FATAL ERROR**********");
        cannot_read_file=true;
        
    }
    
    return index;
    
    
}



void local_search() {
    int k=0;
    int candidate=0;
    int candidate_line=0;
    int try;
    int index;
    int try_next=0;
    int max_try_index=0;
    bool improvement = true;
    bool new_try=false;
    criteria->calibration=1;
    copySolutionToBest();
    while (improvement==true) {
        improvement=false;
        copySetToMini();
        subsets_lottery(0);
        
        //remove lottery subsets
        for(k = 0; k < solution->elem_changed; k++){
            remove_subset(miniSet, solution->lottery_changed[k]);
        }
        
        Column_elimination_mini(miniSet);
        copySolutionTominiSolution();
        copyMiniToNano();
        try=1; try_next=0;
        index=sort_index_frequency(nanoSet,try_next+1);
        max_try_index=subset_frequency(nanoSet, index); //printf("\nmaximo de tentativas:[%d] ", max_try_index);
        
        
        while((improvement==false) && (try<=max_try_index)){
            copyminiSolution_To_Solution();  //printf("\ntentativa:{{[%d]}}\n", try);
            if(cannot_read_file==true) break;
            
            //algorithm begins
            while(checkSolution(solution)==false){
                if(cannot_read_file==true) break;
                
                if((nanoSet->nsurvived==0 || reset_lottery_bool==true)  && checkSolution(solution)==false){
                    reset_lottery_bool=false;  //printf("\n\n *******RESTART - Iterations: %d ******* \n", criteria->global_iterations);
                    new_try=true;
                    copyMiniToNano();
                    copyminiSolution_To_Solution();
                    try_next++;
                    index=sort_index_frequency(nanoSet, try_next);
                    max_try_index=subset_frequency(nanoSet, index);
                    //try=1;
                    
                }
                
                if(new_try==false){
                    index=sort_index_frequency(nanoSet,1); //printf("\nindex:%d ", index);
                }new_try=false;
                
                if(candidate_line==1){
                    candidate=find_subset_index(nanoSet, index, try); //printf("candidate:{{%d}} tam: %d ", candidate, set->nrows_covered[candidate]);
                }else{
                    candidate=find_subset_index(nanoSet, index, 1); //printf("candidate:{{%d}} tam: %d ", candidate, set->nrows_covered[candidate]);
                }
                
                add_subset(nanoSet,candidate);
                Column_elimination_nano(nanoSet);
                
                
                
                //check if current solution is greater than best local
                if((checkSolution(solution)==true)  && (solution->SolutionCost < bestSolution->SolutionCost)){
                    copySolutionToBest();
                    criteria->calibration=criteria->calibration*criteria->calibration_buffer;
                    improvement=true;
                }
                candidate_line++;
            }
            candidate_line=1;
            copyMiniToNano();
            criteria->global_iterations++;
            try++;
        }
    }
    
    ///end of algorithm
    //criteria->solutions_value[criteria->global_iterations]=solution->SolutionCost;
    copyBestTosolution();
    if(bestSolution->SolutionCost < OptimalSolution->SolutionCost)
        copyBestSolutionToOptimal();
    
}


//(8th block)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void disturbance(){
    int k=0;
    int candidate=0;
    int try, try_next=0;
    int index;
    bool new_try=false;
    copySetToMini();
    subsets_lottery(criteria->calibration);
    
    //remove lottery subsets
    for(k = 0; k < solution->elem_changed; k++)
        remove_subset(miniSet, solution->lottery_changed[k]);
    Column_elimination_mini(miniSet);
    copySolutionTominiSolution();
    copyMiniToNano();
    
    
    
    try=1; try_next=0;
    //algorithm begins
    while(checkSolution(solution)==false){
        if(cannot_read_file==true)break;
        index=sort_index_frequency(nanoSet,try_next+1);
        candidate=find_subset_index(nanoSet, index, try);
        
        
        if((nanoSet->nsurvived==0 || reset_lottery_bool==true)  && checkSolution(solution)==false){
            if(cannot_read_file==true)break;
            reset_lottery_bool=false;  //printf("\n\n *******RESTART DISTURBANCE - Iterations: %d ******* \n", criteria->global_iterations);
            new_try=true;
            copySetToNano();
            copyminiSolution_To_Solution();
            try_next++;
            try=1;
            index=sort_index_frequency(nanoSet, try_next);
            candidate=find_subset_index(nanoSet, index, try);
     
        }
        

        if(new_try==false){
            index=sort_index_frequency(nanoSet,1); //printf("\nindex:%d ", index);
        }new_try=false;
        
        
        add_subset(nanoSet,candidate);
        Column_elimination_nano(nanoSet);
        
        
        
        //check current solution is the best solution during current iteration
        if((checkSolution(solution)==true)  && (solution->SolutionCost < bestSolution->SolutionCost)){
            copySolutionToBest();
        }    if(cannot_read_file==true)break;
    }
    if(bestSolution->SolutionCost < OptimalSolution->SolutionCost)
        copyBestSolutionToOptimal();
    //criteria->solutions_value[criteria->global_iterations]=solution->SolutionCost;
    criteria->global_iterations++;
}


bool acceptance_criteria(void){
    
    if(criteria->global_iterations > criteria->Quit_operarions){
        printf("Quit Criteria Reached!");
        return true;
        
    }
    
    if(criteria->global_iterations >= criteria->global_Max_Iter && criteria->reach_zone==1){
        printf("\nMax_Iter to 99%% reached: %d iterations\n", criteria->global_iterations);
        criteria->acceptance_zone=criteria->acceptance_zone_relaxed;
        criteria->reach_zone++;
    }
    
    if(criteria->global_iterations >= criteria->global_Max_Iter2 && criteria->reach_zone==2){
        printf("\nMax_Iter to 95%% reached: %d Iterations\n", criteria->global_iterations);
        criteria->acceptance_zone=criteria->acceptance_zone_relaxed2;
        criteria->reach_zone++;
    }
    
    
    if((OptimalSolution->SolutionCost*criteria->acceptance_zone) <= criteria->GlobalOptimal_value){
        criteria->Max_Iter=0.05*criteria->global_iterations;
        if (OptimalSolution->SolutionCost_prev!=OptimalSolution->SolutionCost){
            OptimalSolution->SolutionCost_prev=OptimalSolution->SolutionCost;
            printf("Iterations Remaining: %d\n", criteria->Max_Iter);
            criteria->iterations_Optimal_zone=0;
        }
        if(criteria->iterations_Optimal_zone==1){
            printf("\nAcceptance Zone!");
            printf("Iterations: %d \n", (criteria->global_iterations-1));
        }
        
        if(OptimalSolution->SolutionCost == criteria->GlobalOptimal_value){
            printf("Optimal Solution Reached Congratulaions!!");
            printf("Iterations: %d\n", criteria->global_iterations);
            return true;
        }
        if(criteria->iterations_Optimal_zone < criteria->Max_Iter){
            criteria->iterations_Optimal_zone++;
        }
        else {
            printf("Criteria Accepted! ");
            printf("Iterations: %d\n", criteria->global_iterations);
            return true;
        }}
    
    return false;
}

//check Optimal solution
void press_results(void){
    FILE *Instances;
    int i=0,j=0,k=0;
    char write[MAX];
    char file[MAX];
    char ext_name[MAX];
    
    printf("\nOptimal Solution:\n");
    for(i=0; i<set->ncols; i++){
        if(OptimalSolution->subSets[i]==1){
            printf("ind:[%d] ", i);
            for(j=0; j<(set->nrows_covered[i]+2); j++){
                if(j<=2)printf("|");
                printf("%d ",set->subsets[i][j]);k++;
            }printf("\n");
        }
    }
    
    criteria->Optimal_percentage=(criteria->GlobalOptimal_value/OptimalSolution->SolutionCost);
    printf("Instance Name: %s\n", instance_list);
    printf("Instance_Size: %dx%d\n", set->ncols,set->nrows);
    printf("Cost: %d\n", OptimalSolution->SolutionCost);
    printf("Optimal percentage: %f\n", criteria->Optimal_percentage);
    printf("Optimal iteration: %d\n", OptimalSolution->optimal_iteration);
    printf("Size: %d\n", OptimalSolution->SolutionSize);
    printf("Elements: %d\n", (k-(2*OptimalSolution->SolutionSize)));
    printf("Lottery_Bounds: %f\n",criteria->Lottery_Bounds);
    printf("Calibration Buffer: %f\n",criteria->calibration_buffer);
    printf("Acceptance Zone: %f\n",criteria->acceptance_zone);
    printf("Iterations: %d\n",criteria->global_iterations);
    printf("cpu_time_used (s): %f\n",criteria->cpu_time_used);
    printf("\n\n***********************************\n\n\n");
    
    
    strcpy(ext_name, "Results: ");
    strcpy(file, instance_list);
    strcat(ext_name,file);
    strcpy(write, file_name_write);
    strcat(write, ext_name);
    Instances = fopen(write, "w");
    fprintf(Instances, "Instance_Name: %s\n", instance_list);
    fprintf(Instances, "Instance_Size: %dx%d\n", set->ncols,set->nrows);
    fprintf(Instances, "Cost: %d\n", OptimalSolution->SolutionCost);
    fprintf(Instances, "Optimal_Percentage: %f \n", criteria->Optimal_percentage);
    fprintf(Instances, "Optimal iteration: %d\n", OptimalSolution->optimal_iteration);
    fprintf(Instances, "Acceptance_Zone: %f\n",criteria->acceptance_zone);
    fprintf(Instances, "Solution size: %d\n", OptimalSolution->SolutionSize);
    fprintf(Instances, "Elements: %d\n", (k-(2*OptimalSolution->SolutionSize)));
    fprintf(Instances, "Lottery_Bounds: %f\n",criteria->Lottery_Bounds);
    fprintf(Instances, "Calibration Buffer: %f\n",criteria->calibration_buffer);
    fprintf(Instances, "Iterations: %d\n",criteria->global_iterations);
    fprintf(Instances, "Cpu_Time_Used(s): %f\n",criteria->cpu_time_used);
    
    
    if(check_solution==true){
        fprintf(Instances,"\n\n***********************************\n\n\n");
        fprintf(Instances, "\nOptimal Solution:\n");
        for(i=0; i<set->ncols; i++){
            if(OptimalSolution->subSets[i]==1){
                fprintf(Instances,"ind:[%d] ", i);
                for(j=0; j<(set->nrows_covered[i]+2); j++){
                    if(j<=2)fprintf(Instances,"|");
                    fprintf(Instances,"%d ",set->subsets[i][j]);k++;
                }fprintf(Instances,"\n");
            }}
    }
    
    
    fclose(Instances);
}


void delete_structs(void){
    
    delete_solution();
    copySolutionTominiSolution();
    copySolutionToBest();
    copyBestSolutionToOptimal();
    delete_set();
    copySetToMini();
    copyMiniToNano();
    createCriteriaStruct();
    
    free(set);
    free(miniSet);
    free(nanoSet);
    free(solution);
    free(miniSolution);
    free(bestSolution);
    free(OptimalSolution);
    
    cannot_read_file=false;
    reset_lottery_bool=false;
    reset_instance=false;
    reset_count=0;
    
}

//(End of Program)
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
