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
	if(cmdParts[1]!=""&&!getPath(cmdParts[1],pos,false)){
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

void cd(string cmdParts,unsigned int &pos,bool changePath){
	if(cmdParts=="")return;
	if(!getPath(cmdParts,pos,changePath))cout<<"Invalid directory\n";
}

void type(string* cmdParts,unsigned int pos){
	string filename = path_file(cmdParts[1]);
	if(filename=="")return;
	cd(cmdParts[1],pos,false);
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
	string filename = path_file(cmdParts[1]);
	if(filename=="")return;
	cd(cmdParts[1],pos,false);
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
	string name = path_file(cmdParts[1]);
	if(name=="")return;
	if(nameCheck(name)==false){
		cout<<"Directory name invalid!\n";
		return;
	} 
	cd(cmdParts[1],pos,false);
	if(findFile(pos,true,name)!=0){
		cout<<"Same named file exist!\n";
		return;
	}
	mk(pos,name,true);
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
	string filename = path_file(cmdParts[1]);
	if(filename=="")return;
	cd(cmdParts[1],pos,false);
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

void save(fstream &FATfile){
	fstream t;
	t.open("dossys.img",ios::out|ios::binary);
	for(int i=0; i<SIZE; i++)t.write((char*)&FAT[i],sizeof(char));
}

//为了重用cd函数，把bind放在cmd.hpp里了 
void bind(string* cmdParts,unsigned int pos){
	string::iterator i;
	string inName1,inName2,outName;
	for(i=cmdParts[1].begin();(*i)!='+';i++);
	string inPath1(cmdParts[1].begin(),i);
	string inPath2(i+1,cmdParts[1].end());
	string outPath=cmdParts[2];
	inName1 = path_file(inPath1);
	inName2 = path_file(inPath2);
	outName = path_file(outPath);
	if(inName1==""||inName2==""||outName=="")return;
	if(!nameCheck(outName)){
		cout<<"Invalid file name!\t-"+outName<<endl;
		return;
	}
	unsigned int inPos1=pos, inPos2=pos, outPos=pos;
	cd(inPath1,inPos1,false);
	cd(inPath2,inPos2,false);
	cd(outPath,outPos,false);
	if(findFile(outPos,false,outName)!=0){
		cout<<outName+" has already exist!\n";
		return;
	}
	inPos1 = findFile(inPos1,false,inName1);
	inPos2 = findFile(inPos2,false,inName2);
	if(inPos1==1||inPos1==0){
		cout<<inName1+" not exists or not a text file!\n";
		return;
	}
	if(inPos2==1||inPos2==0){
		cout<<inName2+" not exists or not a text file!\n";
		return;
	}
	mk(outPos,outName,false);
	outPos = findFile(outPos,false,outName);
	assert(outPos!=1&&outPos!=0);
	unsigned int txtInPos1 = entry2pos(inPos1);
	unsigned int txtInPos2 = entry2pos(inPos2);
	unsigned int txtOutPos = entry2pos(outPos);
	unsigned int inSize1=getFileSize(inPos1);
	unsigned int inSize2=getFileSize(inPos2);
	unsigned int outSize=0;
	appendtxt(txtInPos1,txtInPos2,txtOutPos,inSize1,inSize2,outSize);
	FAT[outPos+0x1c] = outSize&0xff;
	FAT[outPos+0x1d] = ((outSize>>8)&0xff);
	FAT[outPos+0x1e] = ((outSize>>16)&0xff);
	FAT[outPos+0x1f] = ((outSize>>24)&0xff);
}

void copy(string* cmdParts,unsigned int pos){
	//cmdParts[1]里有加号，说明是合并操作 
	if(find(cmdParts[1].begin(),cmdParts[1].end(),'+')!=cmdParts[1].end()){
		bind(cmdParts,pos);
		return;
	}
	unsigned pos1=pos, pos2=pos;
	string filename1 = path_file(cmdParts[1]);
	string filename2 = path_file(cmdParts[2]);
	if(!nameCheck(filename2))cout<<"Invalid file name!\t-"+filename2<<endl<<endl;
	cd(cmdParts[1],pos1,false);
	cd(cmdParts[2],pos2,false);
	if(filename1==""||filename2=="")return;
	if(findFile(pos2,false,filename2)!=0){
		cout<<filename2+" has already exist!\n";
		return;
	}
	pos1 = findFile(pos1,false,filename1);
	if(pos1==1||pos1==0){
		cout<<filename1+" not exists or not a text file!\n";
		return;
	}
	mk(pos2,filename2,false);	//先创建输出的文件 
	pos2 = findFile(pos2,false,filename2);
	assert(pos2!=1&&pos2!=0);
	unsigned int txtPos1 = entry2pos(pos1);
	unsigned int txtPos2 = entry2pos(pos2);
	unsigned int size1=getFileSize(pos1);
	unsigned int size2=0;
	copytxt(txtPos1,txtPos2,size1,size2);
	FAT[pos2+0x1c] = size2&0xff;
	FAT[pos2+0x1d] = ((size2>>8)&0xff);
	FAT[pos2+0x1e] = ((size2>>16)&0xff);
	FAT[pos2+0x1f] = ((size2>>24)&0xff);
}

void help(){
	cout<<"支持的常用指令与简单介绍\n\n";
	cout<<"注意：需要接路径或文件名的指令均支持多级目录、绝对或相对路径。\n绝路径以A:\\作为开头,多级目录用反斜杠\\连接\n\n";
	cout<<"tree\t查看全局的目录树\n";
	cout<<"dir\t列出当前目录的内容，支持根目录和子目录，支持dir+有效路径名\n";
	cout<<"cd\t进入指定文件夹\n";
	cout<<"md\t创建文件夹\n";
	cout<<"rd\t删除非空文件夹\n";
	cout<<"type\t查看文本文件\n";
	cout<<"del\t删除文件或整个目录(可非空)\n";
	cout<<"copy\t复制或合并文件（输出文件必须不存在，需要合并的文件用加号+连接,只支持两个文本文件的合并）\n";
	cout<<"cls\t清屏\n";
	cout<<"save\t将本程序对dossys.img的操作进行保存（即覆盖原有dossys.img）";
	cout<<"exit\t退出程序\n\n";
}

#endif
