#include<fstream>
#include<iostream>
#include<string>
#include<algorithm>
#include<vector>
#define SIZE 1474560
#include<stdio.h>
using namespace std;
char FAT[SIZE]; 
string PATH; 
vector<int> freeCluster;	//记录空闲文件的首簇号 

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

//对单个扇区进行dir操作 
void dirCluster(unsigned int pos,int &count,long long int &totalSize){
	for(int j=pos; j!=pos+0x200; j+=0x20){
		if(FAT[j+0xb]==(char)0x27 || FAT[j]==(char)0xe5)continue;		//不显示隐藏文件和已删除文件 
		if(FAT[j+0xb]==0)break;									//FAT表剩余部分为空时停止打印
		bool isdir = true; 
		//打印文件名和文件类型 
		for(int i=0; i<11; i++){
			if((unsigned)FAT[j+i]>126)cout<<'*';
			else cout<<FAT[j+i];
			if(i==7)cout<<'\t';
			if(i>7 && FAT[j+i]!=0x20)isdir=false;
		}
		if(isdir)cout<<"<DIR>\t\t";
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

int getNextCluster(unsigned int pos){
	if(pos<0x4000)return pos/0x200 -30 ;
	if(pos == 0x4200)return 0xfff;
	int nextClusterNum = 0;
	pos /= 0x200;
	pos -= 31;
	pos*=3;
	if(pos%2){
		pos/=2;
		nextClusterNum += ((unsigned int)FAT[0x200+pos]&0xf);
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
			if(FAT[j+0xb]==0)return 0;									//FAT表剩余部分为空时停止检索
			string tmp,name;					
			if(isDry){												//如果要找的是目录
				for(int i=0; i<8; i++){	
					if(FAT[j+i]==0x20)break;
					tmp+=FAT[j+i];
				}
			}
			else {
				for(int i=0; i<11; i++){
					if(FAT[j+i]==0x20){
						if(*(tmp.end()-1)!='.')tmp+='.';
						continue;
					}
					if(i<8&&FAT[j+i]!=0x20)name+=FAT[j+i];
					tmp+=FAT[j+i];
				}
			}
			if(tmp==filename || name==filename){			//检索文件名是否匹配 
				//判断文件类型 
				if((FAT[j+0xb]!=0x10 && isDry) || (FAT[j+0xb]!=0x20 && !isDry))return 1;
				if(name!=""&&tmp!=filename)return 0;
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

void dir(string* cmdParts,unsigned int pos){
	if(cmdParts[1]!=""&&!getPath(cmdParts,pos,false)){
		cout<<"File not found\n";
		return;
	}
	int count = 0;
	long long int totalSize = 0;
	if(pos==0x2600){
		for(unsigned int i=pos; i!=0x4200; i+=0x200)dirCluster(i,count,totalSize);
	}
	else{
		while(1){
			dirCluster(pos,count,totalSize);
			int nextClusterNum = getNextCluster(pos);
			if(nextClusterNum == 0xfff)break;
			else pos = (nextClusterNum+31)*0x200;
		}
	}
	//输出统计信息
	cout<<'\t'<<count<<" file(s)\t\t" ;
	cout<<strNum(totalSize)<<" bytes\n\n";
}

void cmdDecode(string* cmdParts,string cmd){
	int len = cmd.length();
	int j=0;
	for(int i=0; i!=len; i++){
		if(cmd[i]==' '){
			if(cmdParts[j]!="")j++;
		}
		else cmdParts[j]+=cmd[i];
	}
}

void cd(string* cmdParts,unsigned int &pos,bool changePath){
	if(cmdParts[1]=="")return;
	if(!getPath(cmdParts,pos,changePath))cout<<"Invalid directory\n";
}

//展示文件内容									//没做完下一页！！！！
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

void type(string* cmdParts,unsigned int pos){
	string filename = path_file(cmdParts);
	if(filename=="")return;
	cd(cmdParts,pos,false);
	unsigned int filePos = findFile(pos,false,filename);
	if(filePos == 0)cout<<"File not found\t-"<<cmdParts[1]<<'\\'<<filename<<endl;
	else if(filePos == 1)cout<<"Access denied\t-"<<filename<<endl;
	else{
		unsigned int size =  getFileSize(filePos);
		filePos = entry2pos(filePos);
		display(filePos,size);
	}
}

//检查当前目录是否为空 
bool dirempty(unsigned int pos){
	for(unsigned int i=pos; i!=pos+0x200; i+=0x20){
		if(i+0x20==pos+0x200){
			unsigned int cluster = getNextCluster(pos);
			if(cluster==0xfff)return true;
			pos = (cluster+31)*0x200;
			i = pos;
		}
		if(FAT[i]=='.'&&FAT[i+1]==0x20)continue;
		if(FAT[i]=='.'&&FAT[i+1]=='.'&&FAT[i+2]==0x20)continue;
		if(FAT[i]==(char)0xe5||FAT[i]==0)continue;
		else return false;
	}
}

void rd(string* cmdParts,unsigned int pos){
	unsigned int originPos = pos;
	string filename = path_file(cmdParts);
	if(filename=="")return;
	cd(cmdParts,pos,false);
	pos = findFile(pos,true,filename);
	if(pos==0||pos==1){
		cout<<"Invalid path, not directory,\nor directory not empty\n\n";
		return;
	}
	unsigned int filePos = entry2pos(pos);
	if(!dirempty(filePos)){
		cout<<"Invalid path, not directory,\nor directory not empty\n\n";
		return;
	}
	if(filePos == originPos){
		cout<<"Attempt to remove current directory\t-"<<cmdParts[1]<<'\\'<<filename<<endl<<endl;
	}
	FAT[pos] = 0xe5;
	freeCluster.push_back(FAT[pos+0x1a]&0xff + (FAT[pos+0x1b]&0xff)*0x100);
}

//查看文件树 
void tree(unsigned int pos=0x2600,int level=1){
	if(pos==0x2600){
		//先检索文件 
		for(unsigned int j=pos; j<0x4200; j+=32){
			if(FAT[j+0xb]==0)break;
			if(FAT[j]==(char)0xe5)continue;
			if(FAT[j+0xb]==0x20){
				for(int i=0; i<level-1; i++)cout<<"  |\t";
				cout<<"  |----\t";
				//输出文件名 
				for(unsigned i=j; i<j+8; i++){
					if(FAT[i]==0x20)break;
					if((unsigned) FAT[i]>126)cout<<'*';
					else cout<<FAT[i];
				}
				cout<<'.';
				//输出文件类型 
				for(unsigned i=j+8; i<j+11; i++){
					if(FAT[i]==0x20)break;
					if((unsigned) FAT[i]>126)cout<<'*';
					else cout<<FAT[i];
				}
				cout<<endl;
			}	
		}
		//后检索子目录 
		for(unsigned int j=pos; j<0x4200; j+=32){
			if(FAT[j+0xb]==0)break;	
			if(FAT[j]==(char)0xe5)continue;
			if(FAT[j+0xb]==0x10){
				//输出子目录名 
				string tmp;
				for(unsigned i=j; i<j+8; i++){
					if(FAT[i]==0x20)break;
					if((unsigned) FAT[i]>126)tmp+='*';
					else tmp+=FAT[i];
				}
				if(tmp=="."||tmp=="..")continue;
				for(int i=0; i<level-1; i++)cout<<"  |\t";
				cout<<"  |----\t";
				cout<<tmp<<endl;
				unsigned int cluster = FAT[j+0x1a]&0xff + (FAT[j+0x1b]&0xff)*0x100;
				tree((cluster+31)*0x200,level+1);//输出下一级子文件树 
				for(int i=0; i<level; i++)cout<<"  |\t";
				cout<<endl;
			}
		}
	}
	else{
		unsigned int originPos = pos;
		//先检索文件 
		for(unsigned int j=pos; j<pos+0x200; j+=32){
			if(FAT[j]==(char)0xe5)continue;
			if(FAT[j+0xb]==0x20){
				for(int i=0; i<level-1; i++)cout<<"  |\t";
				cout<<"  |----\t";
				//输出文件名
				for(unsigned i=j; i<j+8; i++){
					if(FAT[i]==0x20)break;
					if((unsigned) FAT[i]>126)cout<<'*';
					else cout<<FAT[i];
				}
				cout<<'.';
				//输出文件类型 
				for(unsigned i=j+8; i<j+11; i++){
					if(FAT[i]==0x20)break;
					if((unsigned) FAT[i]>126)cout<<'*';
					else cout<<FAT[i];
				}
				cout<<endl;
			}
			if(FAT[j+0xb]==0 || j+32==pos+0x200){
				unsigned int cluster = getNextCluster(pos);
				if(cluster==0xfff)break;
				pos = (cluster+31)*0x200;
				j = pos;
			}	
		}
		//后检索子目录 
		pos = originPos;
		for(unsigned int j=pos; j<pos+0x200; j+=32){
			if(FAT[j]==(char)0xe5)continue;
			if(FAT[j+0xb]==0x10){
				//输出子目录名 
				string tmp;
				for(unsigned i=j; i<j+8; i++){
					if(FAT[i]==0x20)break;
					if((unsigned) FAT[i]>126)tmp+='*';
					else tmp+=FAT[i];
				}
				if(tmp=="."||tmp=="..")continue;
				for(int i=0; i<level-1; i++)cout<<"  |\t";
				cout<<"  |----\t";
				cout<<tmp<<endl;
				unsigned int cluster = FAT[j+0x1a]&0xff + (FAT[j+0x1b]&0xff)*0x100;
				tree((cluster+31)*0x200,level+1);//输出下一级子文件树 
				for(int i=0; i<level; i++)cout<<"  |\t";
				cout<<endl;
			}
			if(FAT[j+0xb]==0 || j+32==pos+0x200){
				unsigned int cluster = getNextCluster(pos);
				if(cluster==0xfff)break;
				pos = (cluster+31)*0x200;
				j = pos;
			}
		}
	}
}

void findFreeCluster(unsigned int pos=0x2600){
	for(unsigned int i=pos; i<pos+0x200; i+=32){
		if(FAT[i]=='.'&&FAT[i+1]==0x20)continue;
		if(FAT[i]=='.'&&FAT[i+1]=='.'&&FAT[i+2]==0x20)continue;
		if(FAT[i]==(char)0xe5){
			freeCluster.push_back(FAT[i+0x1a]&0xff + (FAT[i+0x1b]&0xff)*0x100);
			continue;
		}
		if(FAT[i+0xb]==0x10){
			int cluster = FAT[i+0x1a]&0xff + (FAT[i+0x1b]&0xff)*0x100;
			findFreeCluster((cluster+31)*0x200);
		}
		if(FAT[i+0xb]==0)return;
	}
	if(pos<0x4200)findFreeCluster(pos+0x200);
	else{
		int cluster = getNextCluster(pos);
		if(cluster==0xfff)return;
		findFreeCluster((31+cluster)*0x200);
	}
}

void FATinit(fstream &FATfile){
	FATfile.open("test.img",ios::in|ios::out|ios::binary);
	for(int i=0; i<SIZE; i++)FATfile.read((char*)&FAT[i],sizeof(char));
	PATH = "\\";
	findFreeCluster();
}

int main(){
	fstream FATfile;
	string cmd;
	unsigned int pos=0x2600;
	FATinit(FATfile);
	while(1){
		cout<<"A:"<<PATH<<'>';
		getline(cin,cmd);
		int len = cmd.length();
		for(int i=0; i!=len; i++)if(cmd[i]>='a'&&cmd[i]<='z')cmd[i]=cmd[i]-'a'+'A';
		string cmdParts[3];
		cmdDecode(cmdParts,cmd);
		if(cmdParts[0]=="")continue;
		else if(cmdParts[0]=="DIR")dir(cmdParts,pos); 
		else if(cmdParts[0]=="CD")cd(cmdParts,pos,true);
		else if(cmdParts[0]=="TREE"){
			tree();
			cout<<endl;
		}
		else if(cmdParts[0]=="RD")rd(cmdParts,pos);
		else if(cmdParts[0]=="TYPE")type(cmdParts,pos);
		else if(cmdParts[0]=="CLS")system("cls");
		else if(cmdParts[0]=="EXIT")return 0;
		//debug用的指令 
		else if(cmdParts[0]=="POS")printf("0x%x\n",pos);			//显示当前地址 
		else if(cmdParts[0]=="FREE")for(int i=0;i<freeCluster.size();i++)cout<<freeCluster[i]<<' ';//显示空闲文件首簇 
		else cout<<"Bad command or file name\n";
	}
	FATfile.close();
}
//dir 10
//hiden 27
//file 20
