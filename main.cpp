//
//  main.cpp
//  VMM
//
//  Created by Jessie on 14/04/2018.
//  Copyright © 2018 Jessie. All rights reserved.
//

#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <getopt.h>
#include <string>
#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <queue>
#include <cstring>
#include <iomanip>

using namespace std;

unsigned long long unmap = 0;
unsigned long long map = 0;
unsigned long long in = 0;
unsigned long long out = 0;
unsigned long long fin = 0;
unsigned long long fout = 0;
unsigned long long zero = 0;
unsigned long long seg = 0;
unsigned long long segpro = 0;
unsigned long long inst_count = 0;
unsigned long long ctx_switches = 0;
unsigned long long cost = 0;

struct Frame{
    int frameid;
    int pid;
    int vpage;
};

struct VMA{
    int s_vpage;
    int e_vpage;
    int w_protected;
    int f_mapped;
};

struct PTE{
    unsigned isvma:            1;
    unsigned valid:            1;
    unsigned referenced:       1;
    unsigned modified:         1;
    unsigned pagedout:         1;
    unsigned write_protected:  1;
    unsigned file_mapped:      1;
    unsigned frameindex:       7;
}page_table[64];


struct Process{
    unsigned long long unmaps;
    unsigned long long maps;
    unsigned long long ins;
    unsigned long long outs;
    unsigned long long fins;
    unsigned long long fouts;
    unsigned long long zeros;
    unsigned long long segv;
    unsigned long long segprot;
    vector<VMA> vma_list;
    PTE page_table[64];
};

struct Instruction{
    char inst;
    int  vpage_index;
};

//Global variable
vector<Frame> frame_table;
int sum_frame = 128;
int ofs = 0;
vector<int> lineidlist;
vector<string> linelist;
vector<VMA> vmalist;
vector<int> vma_sumlist;
vector<Instruction> insline;
vector<Process *> prolist;
vector<Frame *> pagerlist;



//Read input file
void readFile (string filename){
    char instr;
    int index;
    int s;
    int e;
    int w;
    int m;
    int sum_process; //total number of processes
    int sum_vma = 0; //total number of vmas;
    string word;
    ifstream ifile;
    ifile.open(filename.c_str());
    string line;

    while (getline(ifile, line)){
        int lineid = 0;
        if (line[0] == '#'){
        continue;
        }
        else {
            linelist.push_back(line);
            lineidlist.push_back(lineid);
            lineid++;
        }
    }
  
    istringstream token(linelist[0]);
    token >> sum_process;
    for(int j = 1; j < linelist.size(); j++){
        istringstream token(linelist[j]);
        int count = 0;
        
        while(token >> word){
            count++;
        }
        
        if (count == 1){
            istringstream token(linelist[j]);
            token >> sum_vma;
            vma_sumlist.push_back(sum_vma);
        }
        
        else if(count == 4){
            istringstream token(linelist[j]);
            token >> s >> e >> w >> m;
            VMA vma;
            vma.s_vpage = s;
            vma.e_vpage = e;
            vma.w_protected = w;
            vma.f_mapped = m;
            vmalist.push_back(vma);
        }
        
        else if(count == 2){
            istringstream token(linelist[j]);
            token >> instr >> index;
            Instruction instruction;
            instruction.inst = instr;
            instruction.vpage_index = index;
            insline.push_back(instruction);
        }
    }
    
    
    for(int i = 0; i < sum_process; i++){
        Process* p = new Process() ;
        
        //Initialize page table entry for every process.
        for(int k = 0; k < 64; k++){
             p->page_table[k].isvma = 0;
             p->page_table[k].valid = 0;
             p->page_table[k].referenced = 0;
             p->page_table[k].modified = 0;
             p->page_table[k].pagedout = 0;
             p->page_table[k].write_protected = 0;
             p->page_table[k].file_mapped = 0;
             p->page_table[k].frameindex = -1;
        }
        
        for(int j = 0; j < vma_sumlist[i]; j++){
      
            for(int m = vmalist[j].s_vpage ; m < vmalist[j].e_vpage+1; m++){
                p->page_table[m].isvma = 1;
            
                p->page_table[m].write_protected =  vmalist[j].w_protected;
          
                p->page_table[m].file_mapped =  vmalist[j].f_mapped;
             

            }
             p->vma_list.push_back(vmalist[j]);
            
        }
        vmalist.erase(vmalist.begin(),vmalist.begin()+vma_sumlist[i]);
        prolist.push_back(p);
    }
    ifile.close();
}


//Read random file
vector <int> randvals;
void readRfile(string rfilename){
    int size;
    int number;
    string line;
    ifstream ifile;
    ifile.open(rfilename.c_str());
    getline(ifile,line);
    istringstream token(line);
    token >> size;
    while(getline(ifile,line)){
        istringstream token(line);
        token >> number;
        randvals.push_back(number);
    }
    ifile.close();
}

int myrandom(int burs){
    if (ofs >= randvals.size() - 1){
        ofs = 0;
    }
    int random = randvals[ofs++]%burs;
    return random;
}


class Pager{
public:
    virtual Frame* selectFrame() = 0 ;
};

Pager* alg;

int hasframe;
Frame* get_free_frame(int sumframe, Pager* pa ) {
    hasframe = -1;
    Frame* fra = new Frame();
    for(int i = 0 ; i < sumframe; i++ ){
        if(frame_table[i].frameid == -1){
            frame_table[i].frameid = i;
            hasframe = 1;
            *fra = frame_table[i];
            pagerlist.push_back(fra);
            break;
        }
    }
    return fra;
}

class FIFO : public Pager{
public:
   

    Frame *selectFrame(){
        Frame *frame1 = new Frame();
      
            frame1 = pagerlist.front();
            pagerlist.erase(pagerlist.begin());
            pagerlist.push_back(frame1);
        
        return frame1;
    }
private:
   
};


class SndChance : public Pager{
public:
  
    
    Frame* selectFrame() {
        Frame* frame1 = new Frame();
        frame1 = pagerlist.front();
        pagerlist.erase(pagerlist.begin());
        while(prolist[frame1->pid]->page_table[frame1->vpage].referenced != 0){
            prolist[frame1->pid]->page_table[frame1->vpage].referenced = 0;
            pagerlist.push_back(frame1);
               frame1 = pagerlist.front();
            pagerlist.erase(pagerlist.begin());
            
         
        }
          pagerlist.push_back(frame1);
        return frame1;
    }
};

class Random : public Pager{
public:
    Frame *selectFrame(){
        Frame *frame1 = new Frame();
        frame1 = pagerlist[myrandom(sum_frame)];
        return frame1;
        }
    
};

class NRU : public Pager{
public:
    int counter;
    
    NRU(){
        counter = 0;
    }
    
    Frame* selectFrame(){
       
        Frame* frame1 = new Frame();
        vector<Frame *> active[4];
        counter++;
        counter = counter % 10;
        
        for(int i = 0; i < 4; i++){
            active[i].clear();
        }
        
        for(int i = 0; i < pagerlist.size(); i++){
            
            if(prolist[pagerlist[i]->pid]->page_table[pagerlist[i]->vpage].referenced == 0 &&
                       prolist[pagerlist[i]->pid]->page_table[pagerlist[i]->vpage].modified == 0){
                        active[0].push_back(pagerlist[i]);
                    }
            
            else if(prolist[pagerlist[i]->pid]->page_table[pagerlist[i]->vpage].referenced == 0 &&
            prolist[pagerlist[i]->pid]->page_table[pagerlist[i]->vpage].modified == 1){
                        active[1].push_back(pagerlist[i]);
                    }
            
            else if(prolist[pagerlist[i]->pid]->page_table[pagerlist[i]->vpage].referenced == 1 &&
                    prolist[pagerlist[i]->pid]->page_table[pagerlist[i]->vpage].modified == 0){
                        active[2].push_back(pagerlist[i]);
                    }
            
            else if(prolist[pagerlist[i]->pid]->page_table[pagerlist[i]->vpage].referenced == 1 &&
                    prolist[pagerlist[i]->pid]->page_table[pagerlist[i]->vpage].modified == 1){
                        active[3].push_back(pagerlist[i]);
                    }
        }
        
        for(int  i = 0; i < 4; i++){
            if(active[i].size() > 0){
                frame1 = active[i][myrandom(active[i].size())];
                break;
            }
        }

        if(counter == 0){
            for(int i = 0; i<active[2].size(); i++){
                prolist[active[2][i]->pid]->page_table[active[2][i]->vpage].referenced = 0;
            }
            for(int i = 0; i<active[3].size(); i++){
              prolist[active[3][i]->pid]->page_table[active[3][i]->vpage].referenced = 0;
            }
            
        }
        return frame1;
    }
};


class Clock : public Pager{
public:
    int hand;
    
    Clock(){
        hand = 0;
    }
    
    Frame* selectFrame() {
        
        Frame* frame1 = new Frame();
        while(prolist[pagerlist[hand]->pid]->page_table[pagerlist[hand]->vpage].referenced == 1){
            prolist[pagerlist[hand]->pid]->page_table[pagerlist[hand]->vpage].referenced =0;
            hand++;
            if(hand >= sum_frame){
                hand = 0;
            }
        }
        frame1 = pagerlist[hand];
        hand++;
        if(hand >= sum_frame){
            hand = 0;
        }
    
        return frame1;
    }
    
    
};


class Aging : public Pager{
public:
    unsigned long * agebit;
    Aging(){
       agebit  = new unsigned long[sum_frame]();
    }

    Frame* selectFrame() {
        Frame * frame1 = new Frame();
        int frame = 0;
        
        for(int i = 0; i < sum_frame; i++){
            agebit[i] =  agebit[i] >> 1;
            if(prolist[pagerlist[i]->pid]->page_table[pagerlist[i]->vpage].referenced == 1){
                agebit[i] |= (unsigned long long)1 << 31;
                prolist[pagerlist[i]->pid]->page_table[pagerlist[i]->vpage].referenced = 0;
                
            }
            if(agebit[i] < agebit[frame]){
                frame  = i;
            }
        }
        agebit[frame] = 0;
        frame1 = pagerlist[frame];
        return frame1;
    }
};

class MMU{
public:
    void simulation(const vector<Instruction> &insline, vector<Process *> pList,size_t o, size_t p, size_t f, size_t s ){
        int current_pro = -1;
        for(int i = 0; i < insline.size(); i++){
            inst_count++;
            if(o != string::npos){
                cout << i << ": ==> " << insline[i].inst << " " << insline[i].vpage_index << endl;
            }
            
            if(insline[i].inst == 'c'){
                current_pro = insline[i].vpage_index;
                ctx_switches++;
            }
            else{
                // If the page is already present(valid) in frame_table(physical memory), update pte.
                if(  pList[current_pro]->page_table[insline[i].vpage_index].valid == 1){
                   
                        pList[current_pro]->page_table[insline[i].vpage_index].referenced = 1;
                    
                    if(insline[i].inst == 'w'){
                        if(pList[current_pro]->page_table[insline[i].vpage_index].write_protected != 1)
                        pList[current_pro]->page_table[insline[i].vpage_index].modified = 1;
                        else if(pList[current_pro]->page_table[insline[i].vpage_index].write_protected == 1 ){
                            cout << " SEGPROT" << endl;
                            pList[current_pro]->segprot++;
                        }
                    }
                }
                //If the page is not present in frame_table(physical memory), there are two possibilities:
                //a: It is even not in current process's vma, then print out SEGV and move on to next instruction
                //b: It is in vma and allocate frame.
                else if(pList[current_pro]->page_table[insline[i].vpage_index].valid != 1){
                    //possibility a:
                    if(pList[current_pro]->page_table[insline[i].vpage_index].isvma == 0){
                        cout << "  SEGV" << endl;
                        pList[current_pro]->segv++;
                        continue;
                    }
                    //possibility b:
                    else{
                        Frame* fram = get_free_frame(sum_frame, alg);
                        if(hasframe == -1){
                            fram = alg->selectFrame();
                        }
                        
                        if (fram->pid != -1){
                            pList[fram->pid]->page_table[fram->vpage].valid = 0;
                        }
                        if(fram->pid != -1){
                            if( o != string::npos){
                                cout << " UNMAP " << fram->pid <<":" << fram->vpage << endl;
                                pList[fram->pid]->unmaps++;
                            }
                            if(pList[fram->pid]->page_table[fram->vpage].modified == 1 &&
                               pList[fram->pid]->page_table[fram->vpage].file_mapped == 0){
                                if( o != string::npos){
                                    cout << " OUT" << endl;
                                    pList[fram->pid]->outs++;
                                }
                                pList[fram->pid]->page_table[fram->vpage].pagedout = 1;
                                pList[fram->pid]->page_table[fram->vpage].modified = 0;
                                pList[fram->pid]->page_table[fram->vpage].referenced = 0;
                            }
                            else if(pList[fram->pid]->page_table[fram->vpage].modified == 1 &&
                                    pList[fram->pid]->page_table[fram->vpage].file_mapped == 1){
                                if( o != string::npos){
                                    cout << " FOUT" << endl;
                                    pList[fram->pid]->fouts++;
                                }
                                //pList[fram->pid]->page_table[fram->vpage].pagedout = 1;
                                pList[fram->pid]->page_table[fram->vpage].modified = 0;
                                pList[fram->pid]->page_table[fram->vpage].referenced = 0;
                            }
                    
                        }
                        if(pList[current_pro]->page_table[insline[i].vpage_index].pagedout == 0 &&
                           pList[current_pro]->page_table[insline[i].vpage_index].file_mapped == 0){
                            if(o != string::npos ){
                                cout << " ZERO" << endl;
                                pList[current_pro]->zeros++;
                            }
                        }
                        
                        
                        fram->pid = current_pro;
                        fram->vpage = insline[i].vpage_index;
                        frame_table[fram->frameid].pid = current_pro;
                        frame_table[fram->frameid].vpage = insline[i].vpage_index;
                        
                    if(pList[current_pro]->page_table[insline[i].vpage_index].file_mapped == 1 ){
                        if(o != string::npos ){
                        cout << " FIN" << endl;
                        pList[current_pro]->fins++;
                        }
                    }
                    
                    
                    if(pList[current_pro]->page_table[insline[i].vpage_index].file_mapped == 0 &&
                       pList[current_pro]->page_table[insline[i].vpage_index].pagedout == 1){
                        if(o != string::npos ){
                        cout << " IN" << endl;
                        pList[current_pro]->ins++;
                        }
                    }
                    
                    if(o != string::npos){
                        
                        cout << " MAP " << fram->frameid << endl;
                        pList[current_pro]->maps++;
                    }
                        pList[current_pro]->page_table[insline[i].vpage_index].frameindex = fram->frameid;
                        pList[current_pro]->page_table[insline[i].vpage_index].valid = 1;
                        
                        pList[current_pro]->page_table[insline[i].vpage_index].referenced = 1;
                        
                    
                        if(insline[i].inst == 'w'){
                            if(pList[current_pro]->page_table[insline[i].vpage_index].write_protected == 1){
                                cout << " SEGPROT" << endl;
                                pList[current_pro]->segprot++;
                            }
                            if(pList[current_pro]->page_table[insline[i].vpage_index].write_protected != 1)
                            pList[current_pro]->page_table[insline[i].vpage_index].modified = 1;
                        }
                    }
                    }
                }
                
            }
        if(p != string::npos){
        for(int i = 0; i < pList.size(); i++){
            cout << "PT[" << i << "]: " ;
            for(int j = 0; j < 64 ; j++){
                if(pList[i]->page_table[j].valid == 0 && pList[i]->page_table[j].pagedout == 1){
                    cout << "# ";
                    
                }
                else if(pList[i]->page_table[j].valid == 0 && pList[i]->page_table[j].pagedout == 0){
                    cout << "* ";
                }
                else if(pList[i]->page_table[j].valid == 1){
                    cout << j<<":";
                    
                    if(pList[i]->page_table[j].referenced == 1){
                        cout<< "R";
                    }
                    else{
                        cout<< "-";
                    }
                    if(pList[i]->page_table[j].modified == 1){
                        cout<< "M";
                    }
                    else{
                        cout<< "-";
                    }
                    if(pList[i]->page_table[j].pagedout == 1){
                        cout<< "S " ;;
                    }
                    else{
                        cout<< "- " ;
                    }
                    
                }
                
            }
            cout << endl;
        }
        }
        
        if(f != string::npos){
            cout <<"FT: ";
            for (int i = 0; i < sum_frame; i++) {
                if(frame_table[i].pid == -1){
                    cout<<"* ";
                }
                else{
                cout<< frame_table[i].pid<<":"<<frame_table[i].vpage<<" ";
                }
            }
            cout << endl;
        }
        
    
        
        if(s != string::npos){
            for(int i = 0; i < pList.size();i++) {printf("PROC[%d]: U=%llu M=%llu I=%llu O=%llu FI=%llu FO=%llu Z=%llu SV=%llu SP=%llu\n",
                   i,
                   pList[i]->unmaps, pList[i]->maps, pList[i]->ins, pList[i]->outs,
                   pList[i]->fins, pList[i]->fouts, pList[i]->zeros,
                   pList[i]->segv, pList[i]->segprot);
                 unmap += pList[i]->unmaps;
                 map += pList[i]->maps;
                 in += pList[i]->ins;
                 out += pList[i]->outs;
                 fin += pList[i]->fins;
                 fout += pList[i]->fouts;
                 zero += pList[i]->zeros;
                 seg += pList[i]->segv;
                 segpro += pList[i]->segprot;
                
            }
            
            cost = 400*(unmap+map)+3000*(in+out)+2500*(fin+fout)+150*zero+240*seg+300*segpro
            +121*ctx_switches+(inst_count-ctx_switches);
            
            printf("TOTALCOST %llu %llu %llu\n", ctx_switches, inst_count, cost);
        }
    }
    
};





int main(int argc, char * argv[]) {
    
    
    MMU* mmu;
    if (argc == 1){
        cout << "File not found!" << endl;
    }
    
    int c;
    string algo;
    string option;
    string algo_option;
    opterr = 0;
    while ((c = getopt(argc, argv, "a:o:f: ")) !=-1)
    switch(c){
        case 'a':
            algo = string(optarg);
            break;
        case 'o':
            option = string(optarg);
            break;
        case 'f':
            sum_frame = atoi(optarg);
            break;
        case '?' :
            if(optopt == 'a')
            fprintf(stderr, "Option -%c requires an argument. \n", optopt);
            else if(optopt == 'o')
            fprintf(stderr, "Option -%c requires an argument. \n", optopt);
            else if(optopt == 'f')
            fprintf(stderr, "Option -%c requires an argument. \n", optopt);
            else if(isprint(optopt))
            fprintf(stderr,"Unknown option '-%c'. \n", optopt);
            return 1;
        default:
            abort();
}
   
    for(int i = 0; i < sum_frame; i++){
        Frame fr;
        fr.frameid = -1;
        fr.pid = -1;
        fr.vpage = -1;
        frame_table.push_back(fr);
    }
    
    
    switch(algo[0]){
        case'f':
            alg = new FIFO();
            break;
    
        case's':
            alg = new SndChance();
            break;
       
        case'r':
            alg = new Random();
            break;
            
        case'n':
            alg = new NRU();
            break;
        case'c':
            alg = new Clock();
            break;
        case'a':
            alg = new Aging();
            break;
     
    }
    
    size_t found_O  = option.find('O');
    size_t found_P  = option.find('P');
    size_t found_F  = option.find('F');
    size_t found_S  = option.find('S');
    
    readFile(argv[4]);
    readRfile(argv[5]);
    for(int i = 0; i < prolist.size(); i++){
        
        prolist[i]->unmaps = 0;
        prolist[i]->maps = 0;
        prolist[i]->ins = 0;
        prolist[i]->outs = 0;
        prolist[i]->fins = 0;
        prolist[i]->fouts = 0;
        prolist[i]->zeros = 0;
        prolist[i]->segv = 0;
        prolist[i]->segprot = 0;
        
    }
    
    mmu->simulation(insline, prolist, found_O, found_P, found_F, found_S);
    
    return 0;
}























