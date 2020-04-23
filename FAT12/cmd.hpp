#ifndef CMD_HPP
#define CMD_HPP
#include "func.hpp"
using namespace std;

//�鿴�ļ��� 
void tree(unsigned int pos=0x2600,int level=1){
	if(pos==0x2600){
		//�ȼ����ļ� 
		for(unsigned int j=pos; j<0x4200; j+=32){
			if(FAT[j+0xb]==0)break;
			if(FAT[j]==(char)0xe5)continue;
			if(FAT[j+0xb]==0x20){
				for(int i=0; i<level-1; i++)cout<<"  |\t";
				cout<<"  |----\t";
				//����ļ��� 
				for(unsigned i=j; i<j+8; i++){
					if(FAT[i]==0x20)break;
					if((unsigned) FAT[i]>126)cout<<'*';
					else cout<<FAT[i];
				}
				cout<<'.';
				//����ļ����� 
				for(unsigned i=j+8; i<j+11; i++){
					if(FAT[i]==0x20)break;
					if((unsigned) FAT[i]>126)cout<<'*';
					else cout<<FAT[i];
				}
				cout<<endl;
			}	
		}
		//�������Ŀ¼ 
		for(unsigned int j=pos; j<0x4200; j+=32){
			if(FAT[j+0xb]==0)break;	
			if(FAT[j]==(char)0xe5)continue;
			if(FAT[j+0xb]==0x10){
				//�����Ŀ¼�� 
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
				tree((cluster+31)*0x200,level+1);//�����һ�����ļ��� 
				for(int i=0; i<level; i++)cout<<"  |\t";
				cout<<endl;
			}
		}
	}
	else{
		unsigned int originPos = pos;
		//�ȼ����ļ� 
		for(unsigned int j=pos; j<pos+0x200; j+=32){
			if(FAT[j]==(char)0xe5)continue;
			if(FAT[j+0xb]==0x20){
				for(int i=0; i<level-1; i++)cout<<"  |\t";
				cout<<"  |----\t";
				//����ļ���
				for(unsigned i=j; i<j+8; i++){
					if(FAT[i]==0x20)break;
					if((unsigned) FAT[i]>126)cout<<'*';
					else cout<<FAT[i];
				}
				cout<<'.';
				//����ļ����� 
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
		//�������Ŀ¼ 
		pos = originPos;
		for(unsigned int j=pos; j<pos+0x200; j+=32){
			if(FAT[j]==(char)0xe5)continue;
			if(FAT[j+0xb]==0x10){
				//�����Ŀ¼�� 
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
				tree((cluster+31)*0x200,level+1);//�����һ�����ļ��� 
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
	//���ͳ����Ϣ
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
	//����Ǹ�Ŀ¼ 
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
	//����Ǹ��ļ� 
	FAT[pos] = 0xe5;
	freeCluster.push_back(FAT[pos+0x1a]&0xff + (FAT[pos+0x1b]&0xff)*0x100);
}

void save(fstream &FATfile){
	fstream t;
	t.open("dossys.img",ios::out|ios::binary);
	for(int i=0; i<SIZE; i++)t.write((char*)&FAT[i],sizeof(char));
}

//Ϊ������cd��������bind����cmd.hpp���� 
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
	//cmdParts[1]���мӺţ�˵���Ǻϲ����� 
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
	mk(pos2,filename2,false);	//�ȴ���������ļ� 
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

void create(string* cmdParts,unsigned int pos){
	string name = path_file(cmdParts[1]);
	if(name=="")return;
	if(nameCheck(name)==false){
		cout<<"File name invalid!\n";
		return;
	} 
	cd(cmdParts[1],pos,false);
	if(findFile(pos,true,name)!=0){
		cout<<"Same named file exist!\n";
		return;
	}
	mk(pos,name,false);
	pos = findFile(pos,false,name);
	assert(pos!=1&&pos!=0);
	unsigned int txtPos = entry2pos(pos);
	unsigned int size = 0;
	unsigned int i = txtPos;
	string tmp;
	while(getline(cin,tmp)){
		tmp += '\n';
		int len = tmp.length();
		for(int j=0; j<len; j++){
			FAT[i++] = tmp[j];
			size++;
			if(i==txtPos+0x200){
				if(freeCluster.size()){
					vector<int>::iterator iter = freeCluster.begin();
					assignCluster(txtPos/0x200-31,*iter);
					txtPos = ((*iter)+31)*0x200;
					int cluster = getNextCluster(((*iter)+31)*0x200);
					if(cluster == 0xfff)freeCluster.erase(iter); 
					else{
						assignCluster((*iter),0xfff);
						(*iter) = cluster;
					}
					i = txtPos;
				}
				else{
					unsigned int freePos = findFreePos();
					if(freePos==0){
						cout<<"No space left\n";
						return;
					}
					txtPos = i = freePos;
					assignCluster(freePos/0x200-31,0xfff);
				}
			}
		}
	}
	cin.clear();
	FAT[pos+0x1c] = size&0xff;
	FAT[pos+0x1d] = ((size>>8)&0xff);
	FAT[pos+0x1e] = ((size>>16)&0xff);
	FAT[pos+0x1f] = ((size>>24)&0xff);
}

void append(string* cmdParts,unsigned int pos){
	string name = path_file(cmdParts[1]);
	if(name=="")return; 
	cd(cmdParts[1],pos,false);
	pos = findFile(pos,false,name);
	if(pos==0){
		cout<<"File not exist!\n";
		return;
	}
	if(pos==1){
		cout<<"Not a text file!\n";
		return;
	}
	unsigned int size =  getFileSize(pos);
	unsigned int txtPos = entry2pos(pos);
	unsigned int i = display(txtPos,size,false);
	string tmp;
	while(getline(cin,tmp)){
		tmp += '\n';
		int len = tmp.length();
		for(int j=0; j<len; j++){
			FAT[i++] = tmp[j];
			size++;
			if(i==txtPos+0x200){
				if(freeCluster.size()){
					vector<int>::iterator iter = freeCluster.begin();
					assignCluster(txtPos/0x200-31,*iter);
					txtPos = ((*iter)+31)*0x200;
					int cluster = getNextCluster(((*iter)+31)*0x200);
					if(cluster == 0xfff)freeCluster.erase(iter); 
					else{
						assignCluster((*iter),0xfff);
						(*iter) = cluster;
					}
					i = txtPos;
				}
				else{
					unsigned int freePos = findFreePos();
					if(freePos==0){
						cout<<"No space left\n";
						return;
					}
					txtPos = i = freePos;
					assignCluster(freePos/0x200-31,0xfff);
				}
			}
		}
	}
	cin.clear();
	//�Ĵ�С 
	FAT[pos+0x1c] = size&0xff;
	FAT[pos+0x1d] = ((size>>8)&0xff);
	FAT[pos+0x1e] = ((size>>16)&0xff);
	FAT[pos+0x1f] = ((size>>24)&0xff);
	//��ʱ�� 
	time_t now = time(0);
	tm *ltm = localtime(&now);
	unsigned int input_time= ((ltm->tm_hour&0x1f)<<11)+((ltm->tm_min&0x3f)<<5);
	FAT[pos+22] = input_time & 0xff;
	FAT[pos+23] = (input_time >> 8) &0xff;
	unsigned int input_date = (((ltm->tm_year-80)&0x7f)<<9) + (((ltm->tm_mon+1)&0xf)<<5) + (ltm->tm_mday&0x1f);
	FAT[pos+24] = input_date & 0xff;
	FAT[pos+25] = (input_date >> 8) &0xff;
}

void help(){
	cout<<"֧�ֵĳ���ָ����򵥽���\n\n";
	cout<<"ע�⣺��Ҫ��·�����ļ�����ָ���֧�ֶ༶Ŀ¼�����Ի����·����\n��·����A:\\��Ϊ��ͷ,�༶Ŀ¼�÷�б��\\����\n\n";
	cout<<"tree\t�鿴ȫ�ֵ�Ŀ¼��\n";
	cout<<"dir\t�г���ǰĿ¼�����ݣ�֧�ָ�Ŀ¼����Ŀ¼��֧��dir+��Ч·����\n";
	cout<<"cd\t����ָ���ļ���\n";
	cout<<"md\t�����ļ���\n";
	cout<<"rd\tɾ���ǿ��ļ���\n";
	cout<<"type\t�鿴�ı��ļ�\n";
	cout<<"del\tɾ���ļ�������Ŀ¼(�ɷǿ�)\n";
	cout<<"copy\t���ƻ�ϲ��ļ�������ļ����벻���ڣ���Ҫ�ϲ����ļ��üӺ�+����,ֻ֧�������ı��ļ��ĺϲ���\n";
	cout<<"create\t������д���ı��ļ�\n";
	cout<<"append\t���ı��ļ�ĩβ׷��������\n";
	cout<<"cls\t����\n";
	cout<<"save\t���������dossys.img�Ĳ������б��棨������ԭ��dossys.img��";
	cout<<"exit\t�˳�����\n\n";
}

#endif
