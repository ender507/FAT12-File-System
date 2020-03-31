#ifndef FUNC_HPP
#define FUNC_HPP
#include<string>
#include<algorithm>
#include<iostream>
#include<vector>
#include<string>
#include<ctime>
#include<fstream>
#define SIZE 1474560
using namespace std;
#include<stdio.h>
char FAT[SIZE]; 
string PATH; 
vector<int> freeCluster;	//记录空闲文件的首簇号 

//常用函数参考 
string strNum(unsigned long long num);//将数字用逗号间隔开 
unsigned int getFileSize(unsigned int offset);//传入该entry的首地址 
int getNextCluster(unsigned int pos);//获取当前扇区的下一个簇块号 
unsigned int findFile(unsigned pos,bool isDry,string filename);	//在当前目录下寻找文件，返回首地址。找不到返回0，文件类型不匹配返回1
unsigned int entry2pos(unsigned int offset);//传入entry的地址，返回其首簇号对应的地址
bool getPath(string*cmdParts,unsigned int &pos,bool changePATH);//改变当前目录 
string path_file(string* cmdParts);//将路径+文件名拆分开来，返回文件名（含报错信息输出）
bool dirempty(unsigned int pos);//传入目录首地址，检查目录是否为空 
unsigned int findFreePos();//没有空闲文件产生的空闲簇时，在FAT表中搜寻第一个空闲簇，返回其对应地址。无空闲簇返回0 

//将数字用逗号间隔开 
string strNum(unsigned long long num){
	string res;
	int count=0;
	while(num){
		res+=('0'+num%10);
		count++;
		if(count==3){
			count=0;
			res+=',';
		}
		num/=10;
	}
	reverse(res.begin(),res.end());
	if(*(res.begin()))res.erase(res.begin());
	if(res!="")return res;
	else return "0";
}

unsigned int getFileSize(unsigned int offset){
	unsigned int fileSize;
	fileSize = ((unsigned int)FAT[offset+0x1f]&0xff)<<24;
	fileSize += ((unsigned int)FAT[offset+0x1e]&0xff)<<16;
	fileSize += ((unsigned int)FAT[offset+0x1d]&0xff)<<8;
	fileSize += ((unsigned int)FAT[offset+0x1c]&0xff);
	return fileSize;
}

void printTime(unsigned int offset){
	//4位mm
	int time = (unsigned int)FAT[offset+0x18]>>5 & 7;
	time += (((unsigned int)FAT[offset+0x19]&1)<<3);
	if(time<10)cout<<"0";
	cout<<time<<'-';								
	//5位dd
	time = ((unsigned int)FAT[offset+0x18]&0x1F);
	if(time<10)cout<<"0";
	cout<<time<<'-';	 
	//7位yy 
	time = ((unsigned int)FAT[offset+0x19]>>1) & 0x7f;
	time+=80;
	cout<<time%100<<'\t';
	//打印时间
	bool am = true;//am or pm 
	//5位小时
	time = ((unsigned int)FAT[offset+0x17]>>3)&0x1f;
	if(time>12){
		time-=12;
		am=false;
	}
	if(time==12)am=false;
	cout<<time<<':'; 
	//6位分钟 
	time = ((unsigned int)FAT[offset+0x17]&7)<<3;
	time += ((unsigned int)FAT[offset+0x16]>>5)&7;
	cout<<time;
	if(am)cout<<"a";
	else cout<<"p";
}


int getNextCluster(unsigned int pos){
	if(pos<0x4000)return pos/0x200 -30 ;
	if(pos < 0x4200)return 0xfff;
	int nextClusterNum = 0;
	pos /= 0x200;
	pos -= 31;
	pos*=3;
	if(pos%2){
		pos/=2;
		nextClusterNum += (((unsigned int)FAT[0x200+pos]&0xf0)>>4);
		nextClusterNum += (((unsigned int)FAT[0x200+pos+1]&0xff)<<4);
	}
	else{
		pos/=2;
		nextClusterNum += (((unsigned int)FAT[0x200+pos])&0xff);
		nextClusterNum += (((unsigned int)FAT[0x200+pos+1]&0xf)<<8);
	}
	return nextClusterNum;
}

//在当前目录下寻找文件，返回首地址 
unsigned int findFile(unsigned pos,bool isDry,string filename){	//找不到返回0，文件类型不匹配返回1
	while(1){
		for(unsigned int j=pos; j!=pos+0x200; j+=0x20){
			if(FAT[j+0xb]==0x27 || FAT[j]==(char)0xe5)continue;				//略过隐藏文件和已删除文件
			if(FAT[j+0xb]==0)return 0;		//FAT表剩余部分为空时停止检索
			string name;					
			for(int i=0; i<8; i++){
				if(FAT[j+i]==0x20)break;
				name += FAT[j+i];
			}
			if(FAT[j+8]!=0x20){
				name+='.';
				for(int i=8; i<11; i++){
					if(FAT[j+i]==0x20)break;
					name += FAT[j+i];
				}
			}
			if(name==filename){			//检索文件名是否匹配 
				//判断文件类型 
				if((FAT[j+0xb]!=0x10 && isDry) || (FAT[j+0xb]!=0x20 && !isDry))return 1;
				return j;
			}
		}
		int tmp = getNextCluster(pos);
		if(tmp == 0xfff)return 0;
		pos = (tmp+31)*0x200;
	}
}

//输入目录或文件条目的地址，返回其首簇号对应的地址 
unsigned int entry2pos(unsigned int offset){
	unsigned int cluster = FAT[offset+0x1a]&0xff + (FAT[offset+0x1b]&0xff)*0x100;
	if(cluster==0)return 0x2600;
	return (31+cluster)*0x200;
}


//解析单次路径 
bool pathStep(string step,unsigned int &pos,bool changePATH){
	pos = findFile(pos,true,step);
	if(pos==0||pos==1)return false;
	pos = entry2pos(pos); 
	if(changePATH){
		if(step==".."){
			while(*(PATH.end()-1)!='\\')PATH.erase(PATH.end()-1);
			PATH.erase(PATH.end()-1);
		}
		else if(step!="."){
			if(*(PATH.end()-1)!='\\')PATH+="\\";
			PATH+=step;
		}
		if(PATH=="")PATH+="\\";
	}
	return true;
}

//解析长串路径 
bool getPath(string*cmdParts,unsigned int &pos,bool changePATH){
	unsigned int originPos = pos;
	string originPATH = PATH;
	int len = cmdParts[1].length();
	//绝对路径的处理 (路径以A:/开头) 
	if(len>2&&cmdParts[1][0]=='A'&&cmdParts[1][1]==':'&&cmdParts[1][2]=='\\'){
		pos=0x2600;
		cmdParts[1].erase(cmdParts[1].begin());
		cmdParts[1].erase(cmdParts[1].begin());
		cmdParts[1].erase(cmdParts[1].begin());
		len-=3;
		if(changePATH)PATH="\\";
		if(cmdParts[1]=="")return true;
	}
	int i=0;
	string step;
	while(i!=len+1){
		if(cmdParts[1][i]!='\\' && i!=len)step += cmdParts[1][i];
		else{
			if(!pathStep(step,pos,changePATH)){
				pos = originPos;
				PATH = originPATH;
				return false;
			}
			step = "";
		}
		i++;
	}
	return true;
}

//cmdParts[1]拆分成路径+文件名
string path_file(string* cmdParts){
	if(cmdParts[1]==""){
		cout<<"Required parameter missing\n";
		return "";
	}
	string filename;
	while(cmdParts[1].length()){
		if(*(cmdParts[1].end()-1)=='\\'){
			cmdParts[1].erase(cmdParts[1].end()-1);
			break;
		}
		filename += (*(cmdParts[1].end()-1));
		cmdParts[1].erase(cmdParts[1].end()-1);
	}
	reverse(filename.begin(),filename.end());
	return filename;
} 

//对单个扇区进行dir操作 
void dirCluster(unsigned int pos,int &count,long long int &totalSize){
	for(int j=pos; j!=pos+0x200; j+=0x20){
		if(FAT[j+0xb]==(char)0x27 || FAT[j]==(char)0xe5)continue;		//不显示隐藏文件和已删除文件 
		if(FAT[j+0xb]==0)break;									//FAT表剩余部分为空时停止打印
		//打印文件名和文件类型 
		for(int i=0; i<11; i++){
			if((unsigned)FAT[j+i]>126)cout<<'*';
			else cout<<FAT[j+i];
			if(i==7)cout<<'\t';
		}
		if(FAT[11]=0x10)cout<<"<DIR>\t\t";
		else {
			//若文件不是目录则打印大小 
			unsigned int fileSize = getFileSize(j);
			cout<<"\t\t"<<strNum(fileSize)<<"\t";
			totalSize+=fileSize;
		}
		//打印修改时间mm-dd-yy 
		printTime(j);
		count++;
		cout<<endl;
	}
}

//展示文件内容
void display(unsigned int pos,unsigned int size){
	unsigned int num = pos;
	while(size--){
		cout<<FAT[num++];
		if(num==pos+0x200 && size){
			int cluster = getNextCluster(pos);
			if(cluster = 0xfff)return;
			pos = (cluster+31)*200;
			num = pos;
		}
	}
} 

//检查当前目录是否为空 
bool dirempty(unsigned int pos){
	for(unsigned int i=pos; i!=pos+0x200; i+=0x20){
		if(i+0x20==pos+0x200){
			unsigned int cluster = getNextCluster(pos);
			if(cluster==0xfff)return true;
			pos = (cluster+31)*0x200;
			i = pos - 0x20;
		}
		if(FAT[i]=='.'&&FAT[i+1]==0x20)continue;
		if(FAT[i]=='.'&&FAT[i+1]=='.'&&FAT[i+2]==0x20)continue;
		if(FAT[i]==(char)0xe5||FAT[i]==0)continue;
		else return false;
	}
}

//在当前文件夹下寻找空闲位置并返回首地址,找不到返回0 
unsigned int findFreeEntry(unsigned int pos){
	if(pos < 0x4200){
		while(pos != 0x4200){
			if(FAT[pos]==(char)0xe5 || FAT[pos]==0)return pos;
			pos += 0x20;
			if(pos == 0x4200)return 1;//根目录满了返回1 
		}
	}
	else{
		for(int i=pos; i<pos+0x200; i+=0x20){
			if(FAT[i]==(char)0xe5 || FAT[i]==0)return i;
			if(i+0x20 == pos+0x200){
				unsigned int cluster = getNextCluster(pos);
				if(cluster==0xfff)return 0;
				pos = (cluster+31)*0x200;
				i = pos - 0x20;
			}
		}
	}
} 

void makeEntry(string name,unsigned int pos,int cluster,char type,unsigned int size=0){
	int len = name.length(); 
	//命名+拓展名 
	if(name=="."){
		FAT[pos]='.';
		for(int i=1; i<11; i++)FAT[pos+i]=0x20;
	}
	else if(name==".."){
		FAT[pos]=FAT[pos+1]='.';
		for(int i=2; i<11 ;i++)FAT[pos+i]=0x20;
	}
	else{
		int offset=0;
		for(int i=0; i<len&&offset<11 ;i++,offset++){
			if(name[i]=='.'){
				while(offset!=7)FAT[pos+offset++]=0x20;
				continue;
			}
			FAT[pos + offset] = name[i];
		} 
		while(offset<11){
			FAT[pos+offset]=0x20;
			offset++;
		}
	}
	//文件属性 
	FAT[pos+11] = type;
	//保留位 
	for(int i=12; i<22; i++) FAT[pos+i]=' ';
	//创建时间
	time_t now = time(0);
	tm *ltm = localtime(&now);
	unsigned int input_time= ((ltm->tm_hour&0x1f)<<11)+((ltm->tm_min&0x3f)<<5);
	FAT[pos+22] = input_time & 0xff;
	FAT[pos+23] = (input_time >> 8) &0xff;
	//创建日期
	unsigned int input_date = (((ltm->tm_year-80)&0x7f)<<9) + (((ltm->tm_mon+1)&0xf)<<5) + (ltm->tm_mday&0x1f);
	FAT[pos+24] = input_date & 0xff;
	FAT[pos+25] = (input_date >> 8) &0xff;
	//首簇号 
	FAT[pos+26] = cluster & 0xff;
	FAT[pos+27] = (cluster & 0xff00)>>8;
	//文件大小（目录为0） 
	if(type==0x10)for(int i=28; i<32; i++) FAT[pos+i]=' ';
}

void makedir(string name,unsigned int pos,int cluster){
	makeEntry(name,pos,cluster,0x10);
	//写入目录.和.. 
	unsigned int originPos = pos;
	pos = (cluster+31) * 0x200;
	makeEntry(".",pos,cluster,0x10);
	if(originPos<0x4200)makeEntry("..",pos+0x20,0,0x10);
	else makeEntry("..",pos+0x20,originPos/0x200-31,0x10);
	//清空目录对应扇区的全部数据
	for(unsigned int i=pos+0x40; i<pos+0x200; i+=1)FAT[i]=0;
}

//没有空闲文件产生的空闲簇时，在FAT表中搜寻第一个空闲簇，返回其对应地址。无空闲簇返回0 
unsigned int findFreePos(){
	for(unsigned int pos = 0x4200; pos<0x168000; pos+=0x200){
		if(getNextCluster(pos)==0)return pos;
	}
	return 0;
} 

//将簇号在FAT表中的对应位置改为0xfff 
void endCluster(int cluster){
	if(cluster%2){
		cluster /= 2;
		FAT[0x200+cluster] |= 0xf0;
		FAT[0x200+cluster+1] = 0xff;
	}
	else{
		cluster /= 2;
		FAT[0x200+cluster] = 0xff;
		FAT[0x200+cluster+1] |= 0xf;
	}
}
#endif
