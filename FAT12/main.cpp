#include "cmd.hpp"
using namespace std;

//������ָ���ֳɸ������� 
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

//��freeCluster��ʼ�� (�����ļ��״صļ�¼)
void initFreeCluster(unsigned int pos=0x2600){
	for(unsigned int i=pos; i<pos+0x200; i+=32){
		if(FAT[i]=='.'&&FAT[i+1]==0x20)continue;
		if(FAT[i]=='.'&&FAT[i+1]=='.'&&FAT[i+2]==0x20)continue;
		if(FAT[i]==(char)0xe5){
			freeCluster.push_back(FAT[i+0x1a]&0xff + (FAT[i+0x1b]&0xff)*0x100);
			continue;
		}
		if(FAT[i+0xb]==0x10){
			int cluster = FAT[i+0x1a]&0xff + (FAT[i+0x1b]&0xff)*0x100;
			initFreeCluster((cluster+31)*0x200);
		}
		if(FAT[i+0xb]==0)return;
	}
	if(pos<0x4200)initFreeCluster(pos+0x200);
	else{
		int cluster = getNextCluster(pos);
		if(cluster==0xfff)return;
		initFreeCluster((31+cluster)*0x200);
	}
}

//����FAT����ʱ�ĳ�ʼ�� 
void FATinit(fstream &FATfile){
	cout<<"ϵͳ��ʼ��...\n\n";
	FATfile.open("dossys.img",ios::in|ios::binary);
	if(FATfile==NULL){
		cout<<"û���ҵ������ļ���\n���ڱ�����ǰ�ļ����·�����Ϊdossys.img���ļ���\n";
		exit(0);
	}
	for(int i=0; i<SIZE; i++)FATfile.read((char*)&FAT[i],sizeof(char));
	PATH = "\\";
	initFreeCluster();
	cout<<"��������Ϣ��\n";
	cout<<"������:";for(int i=3; i<11; i++)cout<<FAT[i];
	cout<<"\nÿ�������ֽ���:";cout<<((((unsigned) FAT[12]&0xff)<<8)+((unsigned) FAT[11]&0xff));
	cout<<"\nÿ��������:";cout<<(unsigned)FAT[13];
	cout<<"\nFAT����:";cout<<(unsigned)FAT[16];
	cout<<"\n��Ŀ¼����ļ���:";cout<<((((unsigned)FAT[18]&0xff)<<8)+((unsigned)FAT[17]&0xff));
	cout<<"\n��������:";cout<<((((unsigned) FAT[20]&0xff)<<8)+((unsigned) FAT[19]&0xff));
	cout<<"\nÿ��FAT��ռ�е�������:";cout<<((((unsigned) FAT[23]&0xff)<<8)+((unsigned) FAT[22]&0xff));
	cout<<"\n�ļ�ϵͳ����:";for(int i=54; i<62; i++)cout<<FAT[i];
	cout<<"\n\n��ʼ���ɹ���\n֧�ֵ�ָ�����ϸʹ�÷�ʽ����Ĵ���ļ��е�README.txt��README.md\n����help�鿴֧��ָ��ļ򵥽���\n\n";
}

int main(){
	fstream FATfile;
	string cmd;
	unsigned int pos=0x2600;
	FATinit(FATfile);
	FATfile.close();
	while(1){
		cout<<"A:"<<PATH<<'>';
		getline(cin,cmd);
		int len = cmd.length();
		for(int i=0; i!=len; i++)if(cmd[i]>='a'&&cmd[i]<='z')cmd[i]=cmd[i]-'a'+'A';//ָ��ȫ��ת��д
		string cmdParts[5];
		cmdDecode(cmdParts,cmd);
		if(cmdParts[0]=="")continue;
		//ԭ��ָ�� 
		else if(cmdParts[0]=="DIR")dir(cmdParts,pos); 
		else if(cmdParts[0]=="CD")cd(cmdParts[1],pos,true);
		else if(cmdParts[0]=="RD")rd(cmdParts,pos);
		else if(cmdParts[0]=="MD")md(cmdParts,pos);
		else if(cmdParts[0]=="TYPE")type(cmdParts,pos);
		else if(cmdParts[0]=="DEL")del(cmdParts,pos);
		else if(cmdParts[0]=="CLS")system("cls");
		else if(cmdParts[0]=="COPY")copy(cmdParts,pos);
		else if(cmdParts[0]=="CREATE")create(cmdParts,pos);
		else if(cmdParts[0]=="APPEND")append(cmdParts,pos); 
		//ԭ��ָ�� 
		else if(cmdParts[0]=="HELP")help();
		else if(cmdParts[0]=="TREE"){
			tree();
			cout<<endl;
		}
		else if(cmdParts[0]=="SAVE")save(FATfile);
		else if(cmdParts[0]=="EXIT")return 0;
		//debug�õ�ָ�� 
		else if(cmdParts[0]=="POS")printf("0x%x\n",pos);			//��ʾ��ǰ��ַ 
		else if(cmdParts[0]=="FREE"){
			for(int i=0;i<freeCluster.size();i++)cout<<freeCluster[i]<<' ';//��ʾ�����ļ��״� 
			cout<<endl<<"size: "<<freeCluster.size()<<endl;
		}
		else if(cmdParts[0]=="P2C")p2c();		//����pos���ض�Ӧ��cluster��ֵ 
		else if(cmdParts[0]=="FULL")full(cmdParts,pos);//�ڵ�ǰĿ¼����14����Ŀ¼
		else cout<<"Bad command or file name\n";
	}
}
//dir 10
//hiden 27
//file 20
