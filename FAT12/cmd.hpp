#ifndef CMD_HPP
#define CMD_HPP
#include "func.hpp"
using namespace std;

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
				j = pos - 0x20;
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
				j = pos - 0x20;
			}
		}
	}
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

void cd(string* cmdParts,unsigned int &pos,bool changePath){
	if(cmdParts[1]=="")return;
	if(!getPath(cmdParts,pos,changePath))cout<<"Invalid directory\n";
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

void md(string *cmdParts,unsigned int pos){
	string name = path_file(cmdParts);
	if(name=="")return;
	cd(cmdParts,pos,false);
	if(count(name.begin(),name.end(),'.')>1 || findFile(pos,true,name)!=0){
		cout<<"Unable to creat directory\n";
		return;
	}
	unsigned int lastPos = pos, firPos = pos;
	pos = findFreeEntry(lastPos);
	//根目录entry达到最大值，不能创建 
	if(pos == 1){
		cout<<"No space left in root directory\n";
		return;
	}
	//当前目录正好被占满 
	if(pos == 0){
		if(freeCluster.size()){
			vector<int>::iterator iter = freeCluster.begin();
			assignCluster(lastPos/0x200-31,*iter);
			unsigned freePos = (*iter+31)*0x200;
			for(unsigned int i=freePos; i<freePos+0x200; i+=1)FAT[i]=0;
			unsigned int tmp = getNextCluster(((*iter)+31)*0x200);
			if(tmp == 0xfff)freeCluster.erase(iter);//队列头的簇不存在下一个簇，则出队
			//若存在，将该簇的下一个簇号改为0xfff，队列中该位置替换为原来的下簇的簇号 
			else{
				assignCluster((*iter),0xfff);
				(*iter) = tmp;
			}
			if(freeCluster.size()){
				iter = freeCluster.begin();
				makedir(name,firPos,freePos,*iter);
				tmp = getNextCluster(((*iter)+31)*0x200);
				if(tmp == 0xfff)freeCluster.erase(iter); 
				else{
					assignCluster((*iter),0xfff);
					(*iter) = tmp;
				}
			}
			else{
				unsigned int freePos2 = findFreePos();
				makedir(name,firPos,freePos,freePos2/0x200-31);
				assignCluster(freePos2/0x200-31,0xfff);
			}
		}
		else{
			unsigned int freePos = findFreePos();
			if(freePos==0){
				cout<<"No space left\n";
				return;
			}
			int cluster = freePos/0x200-31;
			assignCluster(cluster,0xfff);
			assignCluster((lastPos/0x200)-31,cluster);
			for(unsigned int i=freePos; i<freePos+0x200; i+=1)FAT[i]=0;
			unsigned int freePos2 = findFreePos();
			if(freePos2==0){
				cout<<"No space left\n";
				return;
			}
			assignCluster(freePos2/0x200-31,0xfff);
			makedir(name,firPos,freePos,freePos2/0x200-31);
		}
	}
	else{
		unsigned int cluster = FAT[pos+0x1a]&0xff + (FAT[pos+0x1b]&0xff)*0x100;
		if(freeCluster.size()){
			vector<int>::iterator iter = find(freeCluster.begin(),freeCluster.end(),cluster);
			//当前entry的首簇不在空闲文件簇的队列中,则用队列头的簇作为新文件夹的首簇 
			if(iter==freeCluster.end())iter = freeCluster.begin();
			makedir(name,firPos,pos,*iter);
			unsigned int tmp = getNextCluster(((*iter)+31)*0x200);
			if(tmp == 0xfff)freeCluster.erase(iter);//队列头的簇不存在下一个簇，则出队
			//若存在，将该簇的下一个簇号改为0xfff，队列中该位置替换为原来的下簇的簇号 
			else{
				assignCluster((*iter),0xfff);
				(*iter) = tmp;
			}
		}
		else{
			unsigned int freePos = findFreePos();
			if(freePos==0){
				cout<<"No space left\n";
				return;
			}
			int cluster = freePos/0x200-31;
			assignCluster(cluster,0xfff);
			makedir(name,firPos,pos,cluster);
		}
	}
}

void p2c(){
	int pos;
	scanf("%x",&pos);
	printf("%x\n",getNextCluster(pos));
}

void full(string *cmdParts,unsigned int pos){
	for(int i=0;i!=14;i++){
		cmdParts[1]="";
		cmdParts[1] += ('A'+i);
		md(cmdParts,pos);
	}
}

void del(string *cmdParts,unsigned int pos){
	string filename = path_file(cmdParts);
	if(filename=="")return;
	cd(cmdParts,pos,false);
	unsigned originPos = pos;
	pos = findFile(pos,false,filename);
	if(pos == 0){
		cout<<"File not found\n";
		return;
	}
	//如果是个目录 
	if(pos == 1){
		pos = findFile(originPos,true,filename);
		string tmp="";
		while(tmp==""||(tmp[0]!='y'&&tmp[0]!='Y'&&tmp[0]!='n'&&tmp[0]!='N')){
			cout<<"All files in directory will be deleted!\n";
			cout<<"Are you sure(Y/N)?";
			getline(cin,tmp);
		}
		if(tmp[0]=='N'||tmp[0]=='n')return;
		deleteAll(pos);
		return;
	}
	//如果是个文件 
	FAT[pos] = 0xe5;
	freeCluster.push_back(FAT[pos+0x1a]&0xff + (FAT[pos+0x1b]&0xff)*0x100);
}

#endif
