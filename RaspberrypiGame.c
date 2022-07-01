//cy20202 Amane Inaba  2022/06/07
#include <errno.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/input.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <time.h>

// peripheral register physical address
#define GPIO_PHY_BASEADDR  0x3F200000
#define GPIO_AREA_SIZE 4096 // PAGE_SIZE
#define GPIO_GPFSEL0 0x0000 // for gpio 0..9, MSB 2bits are reserved
// omit GPFSEL1..GPFSEL5
#define GPIO_GPSET0 0x001C // gpio 0..31
#define GPIO_GPSET1 0x0020 // gpio 32..53
#define GPIO_GPCLR0 0x0028 // gpio 0..31
#define GPIO_GPCLR1 0x002C // gpio 32..53
#define GPIO_GPLEV0 0x0034 // gpio 0..31
#define GPIO_GPLEV1 0x0038 // gpio 32..53

int i2c,val,off;
/*シグナルハンドラ*/
void sig_handler(int signum){
 off=1;//グローバル変数への代入を行い，メインのループでその変数をチェックして LCD 制御
};
unsigned int memread(void *baseaddr, int offset);
void memwrite(void *baseaddr, int offset, unsigned int x);

void *gpio_baseaddr;

unsigned int memread(void *baseaddr, int offset)
{
    unsigned int *p;
    p = baseaddr+offset;
    return *p; // read memory-mapped register
}

void memwrite(void *baseaddr, int offset, unsigned int x)
{
    unsigned int *p;
    p = baseaddr+offset;
    *p = x; // write memory-mapped register
}

int lcd_cmdwrite(int fd, unsigned char dat)
{
 unsigned char buff[2];
 buff[0] = 0;
 buff[1] = dat;
 return write(fd,buff,2);
}

int lcd_datawrite(int fd, char dat[])
{
 int len;
 char buff[100];

 len = strlen(dat);  // don't count EOS (Null char)
 if (len>99) {printf("too long string\n"); exit(1); }
 memcpy(buff+1, dat, len); // shift 1 byte, ignore EOS
 buff[0] = 0x40; // DATA Write command
 return write(fd, buff, len+1);
}

void initLCD(int fd)
{
 int i;
 unsigned char init1[]={ 0x38, 0x39, 0x14, 0x70, 0x56, 0x6c };
 unsigned char init2[]={ 0x38, 0x0c, 0x01 };

 usleep(100000); // wait 100ms
 for (i=0;i<sizeof(init1)/sizeof(unsigned char);i++) {
  if(lcd_cmdwrite(fd, init1[i])!=2){
   printf("internal error1\n");
   exit(1);
  }
  usleep(50); // wait 50us
 }

 usleep(300000); // wait 300ms

 for (i=0;i<sizeof(init2)/sizeof(unsigned char);i++) {
  if(lcd_cmdwrite(fd, init2[i])!=2){
   printf("internal error2\n");
   exit(1);
  }
  usleep(50);
 }
 usleep(2000); // wait 2ms
}

int location(int fd, int y)
{
 int x = 0;
 int cmd=0x80 + y * 0x40 + x;
 return lcd_cmdwrite(fd, cmd);
}

int clear(int fd)
{
 int val = lcd_cmdwrite(fd, 1);
 usleep(1000); // wait 1ms
 return val;
}
int random_gen()
{
	int random;
	srand((unsigned int)time(NULL));
    // 0から5までの乱数を発生
   random = rand() % 6;
   return random;
}
/*
	clearとclear関数のエラー処理も同時に行う関数
*/
void e_clear(){
	val=clear(i2c);//LCD上の表示を削除
	if(val==-1)printf("clear error");
}
/*
	引数は
	戻り値はどの色のタクトスイッチを押したかを判別する整数型の値
	タクトスイッチが押されていないか、どの色が押されているかを判別する関数
*/
int judgeButton(unsigned int x){
	if((x&0x00400000)==0)//preesed.c
	{
		printf("pushed blacked\n");
		return 0;
	}
	else if((x&0x00800000)==0)//preesed.c
	{
		printf("pushed red\n");
		return 1;
	}
	else if((x&0x01000000)==0)//preesed.c
	{
		printf("pushed orange\n");
		return 2;
	}
	else if((x&0x02000000)==0)//preesed.c
	{
		printf("pushed yellow\n");
		return 3;
	}
	else if((x&0x04000000)==0)//preesed.c
	{
		printf("pushed green\n");
		return 4;
	}
	else if((x&0x08000000)==0)//preesed.c
	{
		printf("pushed blue\n");
		return 5;
	}
	else{
		printf("un pushed\n");
		return 6;
	}
}
/*
戻り値なし、返り値なし
i2cの基本設定をする関数
*/
void i2csetup(){

 	i2c=open("/dev/i2c-1", O_RDWR);
 	if(i2c==-1)printf("open error");
 	val=ioctl(i2c, I2C_SLAVE, 0x3e); // I2C 独自の設定
	if(val==-1)printf("ioctl error");
	initLCD(i2c); // lcd.c の中に定義されている（戻り値なし）
}
/*
戻り値は整数型で、三秒カウントの間なにも押さない場合は６を、タクトスイッチが
押された場合はスイッチに応じた整数を返す関数
*/
int playAgain(){
		int x,z=6,l=3;
		e_clear();
		location(i2c,0);
	  	val=lcd_datawrite(i2c, "Again?");	
	  	if(val==-1)printf("datawrite error");
	  	location(i2c,1);
	  	while(1){
		 	if(off==1){
		 		if(l==3){
		 	 		val=lcd_datawrite(i2c,"3");//helloをLCD上に表示
					if(val==-1)printf("datawrite error"); 
	 				l-=1;	 
		 		}
		 		else if(l==2){
		 			e_clear();
					location(i2c,0);
	  				val=lcd_datawrite(i2c, "Again?");	
	  				if(val==-1)printf("datawrite error");
	  				location(i2c,1);
	 		 		val=lcd_datawrite(i2c,"2");//helloをLCD上に表示
					if(val==-1)printf("datawrite error"); 
		 			l-=1;
		 		}
		 		else if(l==1){
		 			location(i2c,0);
	  				val=lcd_datawrite(i2c, "Again?");	
	  				if(val==-1)printf("datawrite error");
	  				location(i2c,1);
		  		 	val=lcd_datawrite(i2c,"1");//helloをLCD上に表示
					if(val==-1)printf("datawrite error"); 
		 			l-=1;		
		 		}
		 		else if(l==0)break;
		 			
		 	}
		 	off=0;
		  	x = *((unsigned int *)(gpio_baseaddr + GPIO_GPLEV0));
		  	location(i2c,0);
		  	if(judgeButton(x)!=6)z=judgeButton(x);
		}
		return z;
		



}

/*
戻り値はどのタクトスイッチが押されたかがわかる整数
三秒のカウントダウンの間にどのタクトスイッチが押されたかを判別する関数
*/
int CountDown(){
	int l=3,answer=6;
	unsigned int x;
	 	while(1){
	 		if(off==1){
	 			if(l==3){
	 				e_clear();
	 		 		val=lcd_datawrite(i2c,"3");
					if(val==-1)printf("datawrite error"); 
	 				l-=1;
	 			}
	 			else if(l==2){
	 				e_clear();
	 		 		val=lcd_datawrite(i2c,"2");
					if(val==-1)printf("datawrite error"); 
	 				l-=1;
	 			}
	 			else if(l==1){
	 				e_clear();
	  		 		val=lcd_datawrite(i2c,"1");
					if(val==-1)printf("datawrite error"); 
	 				l-=1;		
	 			}
	 			else if(l==0)break;
	 			
	 		}
	 		off=0;
	  		x = *((unsigned int *)(gpio_baseaddr + GPIO_GPLEV0));
	  		if(judgeButton(x)!=6)answer=judgeButton(x);
	  	}
	  	return answer;
}
/*
戻り値、引数はなし
ＬＣＤの上側に"Press"ＬＣＤの下側に"Button"を表示させる関数
*/
void PressDisplay(){
	 	location(i2c,0);
	 	val=lcd_datawrite(i2c, "Press");//helloをLCD上に表示
		if(val==-1)printf("datawrite error"); 	
		location(i2c,1);
		val=lcd_datawrite(i2c,"Button");
		if(val==-1)printf("datawrite error");
}
/*
戻り値はなし、引数はint型で二つ、randomは正解色と対応する整数、
answerはユーザーが押したタクトスイッチの色と対応する整数
ユーザーがおしたタクトスイッチが正解かどうかを判断する関数
*/
void CheckAnswer(int random,int answer){
	 	if(random==answer){
	 		location(i2c,0);
	  		val=lcd_datawrite(i2c, "You");	
	  		if(val==-1)printf("datawrite error");
	  		location(i2c,1);
	  		val=lcd_datawrite(i2c, "Won!");	
	  		if(val==-1)printf("datawrite error");

	  		sleep(3);
	 	}
	 	else if(random!=answer){
	  		val=lcd_datawrite(i2c, "Miss!");	
	  		if(val==-1)printf("datawrite error");
	  		location(i2c,1);
	  		if(random==0){
	  			val=lcd_datawrite(i2c, "Black");	
	  			if(val==-1)printf("datawrite error"); 		
	  		}
	  		else if(random==1){
	  			val=lcd_datawrite(i2c, "Red");	
	  			if(val==-1)printf("datawrite error"); 		
	  		}
	  		else if(random==2){
	  			val=lcd_datawrite(i2c, "Orange");	
	  			if(val==-1)printf("datawrite error"); 		
	  		}
	  		else if(random==3){
	  			val=lcd_datawrite(i2c, "Yellow");	
	  			if(val==-1)printf("datawrite error"); 		
	  		}
	  		else if(random==4){
	  			val=lcd_datawrite(i2c, "Green");	
	  			if(val==-1)printf("datawrite error"); 		
	  		}
	  		else if(random==5){
	  			val=lcd_datawrite(i2c, "Blue");	
	  			if(val==-1)printf("datawrite error"); 		
	  		}
	  		sleep(3);	
	 	}

}
/*
戻り値引数はなし
ＬＣＤの上側に"Play"ＬＣＤの下側に"Again!!"を表示させる関数
*/
void AgainDisplay(){
	e_clear();
	location(i2c,0);
	val=lcd_datawrite(i2c, "Play");
	if(val==-1)printf("datawrite error");	
	location(i2c,1);
	val=lcd_datawrite(i2c, "Again!!");	
	if(val==-1)printf("datawrite error");
	sleep(3);
	e_clear();

}
/*
戻り値、引数なし
ＬＣＤの上側に"GameEnd"ＬＣＤの下側に"goodBye!"を三秒間
表示させi2cを閉じる関数
*/
void GameEnd(){
	e_clear();
	location(i2c,0);
	val=lcd_datawrite(i2c, "GameEnd");//helloをLCD上に表示	
	if(val==-1)printf("datawrite error");	
	location(i2c,1);
	val=lcd_datawrite(i2c, "GoodBye!");//helloをLCD上に表示	
	if(val==-1)printf("datawrite error");
	sleep(3);
	e_clear();
	val=close(i2c);
	if(val==-1)printf("close error");
}
/*
テスト結果
実際に実行してみると
黒のボタンを一回押すとlcd上にhelloが表示される。これはボタンを離さない限り、表示され続ける
ボタンを離すと一秒後にLCD上には何も表示されなくなり、プログラムが終了する。
*/
int main(){
	while(1){
		unsigned int x,y;
		int i,l=3;
		int fd,random,r,answer=6;
		random=random_gen();
		signal(SIGALRM,sig_handler);/*タイマーの設定*/
		struct itimerval timval;
		timval.it_interval.tv_sec=1;
		timval.it_interval.tv_usec=0;
		timval.it_value.tv_sec=1;//間隔は一秒
		timval.it_value.tv_usec=0;
		fd = open("/dev/mem", O_RDWR);
	 	if(fd==-1)printf("open error");
	 	gpio_baseaddr = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO_PHY_BASEADDR);
	 	i2csetup();
	 	setitimer(ITIMER_REAL,&timval,NULL);//timer start
	 	while(1){
	 		x = *((unsigned int *)(gpio_baseaddr + GPIO_GPLEV0));
	 		r=judgeButton(x);
	 		if(r!=6)break;
	 		PressDisplay();
			if(off==1){
				e_clear();
				usleep(500000);
				off=0;
			}
	 	}
		e_clear();
		answer=CountDown();
	 	e_clear();
		CheckAnswer(random,answer);
	 	val=playAgain();
	 	if(val==6)break;
		AgainDisplay();
	}
	GameEnd();
	
	return 0;
} 
