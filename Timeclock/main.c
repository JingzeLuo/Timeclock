#include <msp430.h> 
#include "lib\key_button.h"
#include "lib\oled.h"
#include "lib\bmp.h"

/*������*/
/*2020302121241 �޾���*/

//AD�¶���ʾ
#define CALADC12_15V_30C  *((unsigned int *)0x1A1A)   // Temperature Sensor Calibration-30 C
                                                      //See device datasheet for TLV table memory mapping
#define CALADC12_15V_85C  *((unsigned int *)0x1A1C)   // Temperature Sensor Calibration-85 C

//function
void SMCLK_XT2_4Mhz(void);
void LEDflash(void);  //led��˸
void select_function(void);  //����ѡ��
void oledshow_time(void);  //��ʾʱ�������
void settime(void);   //����ʱ��
void alarmclock_set(void);  //��������
void alarmclock(void);  //���Ӷ���
void timer(void);  //��ʱ��
  //flash�洢
void read_timedata(void);
void write_flash_int (unsigned int addr, int *array, int count);            // ��int���ͽ���дflash
void read_flash_int (unsigned int addr, int *array, int count);              // ��int���ͽ��ж�flash
  //AD�¶���ʾ
void temp_set(void);
void temp_show(void);

//variable
int hour=0, min=0, second=0; //��ʾʱ��
int year=0, month=0, day=0;  //����
int week=0;  //����
char Tstring[]="Time:";  //ʱ�俪ͷ���ַ�
char AM[]="AM", PM[]="PM";
char ON[]="ON", OFF[]="OFF";
int key_value=0;  //��ȡ����ֵ
int modex_flag=0; //ʱ��ģʽת����־λ 0��24Сʱ��  1��12Сʱ��
int timeset_mod12_APflag=0;  //ʱ������12ʱ��ģʽAP��־λ
int i=0;  //timeset�ļ���λ
int j,k;  //forѭ���ڼ���
int timeset[2][10]={0};  //����ʱ������  timeset[0]Ϊʱ�䣬timeset[1]Ϊ����
int flashtime[8]={0};   //����д��flash
int alarmset[3][8]={0};    //��������ʱ��
int alarm_hour[3]={0}, alarm_min[3]={0}, alarm_second[3]={0};   //����ʱ��
int u=0;  //alarmset�ļ���λ
int alarm_flag[3]={0};  //����ʹ�ܵı�־λ
int alarm_ex=0;    //�л����Ӽ���λ
int alarm_light_timecount=0;  //���ӵ���ʱ����˸����
int alarm_light_enable=0;   //���ӵ�����˸ʹ��
int timer_enable=0;  //��ʱ��ʹ��
int timer_hour=0, timer_min=0, timer_second=0; //��ʱ��ʱ��
  //AD�¶���ʾ����
unsigned int temp;
volatile float temperatureDegC;
volatile float temperatureDegF;
int temtemp = 0;

int main(void)
{
  	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer

 	read_timedata();  //��ȡflash�е�����
 	LEDflash();  //LED��˸�����������ã���ʱ�������ú��ж�ʹ�ܣ�
	temp_set();  //��ȡ�¶ȵ�����
 	OLED_Init(); /*init OLED*/
	OLED_Clear();  /*clear OLED screen*/

	init_key();/*init keyboard*/

	while(1)
	{
	    oledshow_time();  //��ʾʱ������
	    select_function();  //����ѡ��
	    temp_show();   //�¶���ʾ
	}
}

void SMCLK_XT2_4Mhz(void)
{

    P7SEL |= BIT2+BIT3;                       // Port select XT2
    UCSCTL6 &= ~XT2OFF;          // Enable XT2
    UCSCTL6 &= ~XT2OFF + XT2DRIVE_1;          // Enable XT2
    UCSCTL3 |= SELREF_2;                      // FLLref = REFO
                                              // Since LFXT1 is not used,
                                              // sourcing FLL with LFXT1 can cause
                                              // XT1OFFG flag to set
    UCSCTL4 |= SELA_2;                        // ACLK=REFO,SMCLK=DCO,MCLK=DCO

    // Loop until XT1,XT2 & DCO stabilizes - in this case loop until XT2 settles
    do
    {
      UCSCTL7 &= ~(XT2OFFG + XT1LFOFFG + XT1HFOFFG + DCOFFG);
                                              // Clear XT2,XT1,DCO fault flags
      SFRIFG1 &= ~OFIFG;                      // Clear fault flags
    }while (SFRIFG1&OFIFG);                   // Test oscillator fault flag

    UCSCTL6 &= ~XT2DRIVE0;                    // Decrease XT2 Drive according to
                                              // expected frequency
    UCSCTL4 |= SELS_5 + SELM_5;               // SMCLK=MCLK=XT2
}
void LEDflash(void)
{
    SMCLK_XT2_4Mhz();//����SMCLKʹ��XT2�� Ƶ��Ϊ4MHz
    UCSCTL5|=DIVS__32;//ʹ��USCͳһʱ��ϵͳ����Ԥ��Ƶ����SMCLK����32��Ƶ

    TA0CTL |= ID_2 + TASSEL_2 + MC_1 + TACLR;//����A0��������4��Ƶ��ʱ��ԴSMCLK������ģʽ��ͬʱ���������
    TA0EX0 |= TAIDEX_4;//A0��������Ƶ��5��Ƶ
    TA0CCTL0 = OUTMOD_2+CCIE;//����ȽϼĴ���0��������ģʽΪ2��ͬʱʹ�ܶ�ʱ���жϣ�CCR0��Դ�жϣ�
    TA0CCR0 = 6250;//����ȽϼĴ���0����ֵΪ6250
    __bis_SR_register(GIE);//ʹ��ȫ���ж�
    P5DIR |= BIT0;
}
#pragma vector = TIMER0_A0_VECTOR//TA0CCR0�жϷ�����
__interrupt void TIMER0_A0_ISR(void)
{
    P5OUT ^= BIT0;

    if(hour==23&&min==59&&second==59)
    {
        if(month==12&&day==31)
        {
            day = 1;
            month = 1;
            year = year + 1;//���λ
        }
        else if(((month==1||month==3||month==5||month==7||month==8||month==10)&&day==31)
        ||((month==4||month==6||month==9||month==11)&&day==30)
        ||((month==2 && year%4!=0)&&day==28)
        ||((month==2 && year%4==0)&&day==29))
        {
            day=1;
            month = month+1; //�½�λ
        }
        else day=day+1;//�ս�λ
    }

    if(min==59&&second==59) hour = (hour+1)%24;
    if(second==59) min = (min+1)%60;
    second = (second+1)%60;  //ʱ����ʱ

    alarmclock();  //���Ӷ���

    if(timer_enable==1)
    {
        if(timer_min==59&&timer_second==59) timer_hour = timer_hour+1;
        if(timer_second==59) timer_min = (timer_min+1)%60;
        timer_second = (timer_second+1)%60;
    }  //��ʱ����ʱ
}

void oledshow_time(void)
{
    OLED_ShowString(2, 0, Tstring);  //Time:��ʱ�䣩
    if(second>=0 && second<60) OLED_ShowNum(90, 0, second, 2, 16);
    else OLED_ShowString(90, 0, "**");
    OLED_ShowChar(82, 0, ':');
    if(min>=0 && min<60) OLED_ShowNum(66, 0, min, 2, 16);
    else OLED_ShowString(66, 0, "**");
    OLED_ShowChar(58, 0, ':');
    if(modex_flag==0)
    {
      if(hour>=0 && hour<24) OLED_ShowNum(42, 0, hour, 2, 16);
      else OLED_ShowString(42, 0, "**");
      OLED_ShowChar(106, 0, ' ');
      OLED_ShowChar(114, 0, ' ');
    }  //24ʱ��
    else if(modex_flag==1)
    {
        if(hour<12&&hour>=0)
        {
            OLED_ShowNum(42, 0, hour, 2, 16);
            OLED_ShowString(106, 0, AM);
        }
        else if(hour==12)
        {
            OLED_ShowNum(42, 0, hour, 2, 16);
            OLED_ShowString(106, 0, PM);
        }
        else if(hour>12&&hour<24)
        {
            OLED_ShowNum(42, 0, hour-12, 2, 16);
            OLED_ShowString(106, 0, PM);
        }
        else
        {
            OLED_ShowChar(42, 0, '*');
            OLED_ShowChar(50, 0, '*');
            OLED_ShowString(106, 0, "**");
        }
    }  //12ʱ��
    OLED_ShowCHinese(2,2,26);
    OLED_ShowCHinese(18,2,27);
    OLED_ShowChar(34, 2, ':');  //���ڣ�
    if(year>=0) OLED_ShowNum(42, 2, year, 4, 16);
    else OLED_ShowString(42, 2, "****");
    OLED_ShowChar(74, 2, '.');
    if(month>0 && month<13) OLED_ShowNum(82, 2, month, 2, 16);
    else OLED_ShowString(82, 2, "**");
    OLED_ShowChar(98, 2, '.');
    if((((month==1||month==3||month==5||month==7||month==8||month==10||month==12)&&day<32)
            ||((month==4||month==6||month==9||month==11) && day<31)
            ||(month==2 && year%4==0 && day<30)
            ||(month==2 && year%4!=0 && day<29))
            && day>0)
        OLED_ShowNum(106, 2, day, 2, 16);
    else OLED_ShowString(106, 2, "**");

    if(year%4==0)
    {
        OLED_ShowCHinese(82,4,53);
        OLED_ShowCHinese(98,4,55);
    }
    if(year%4!=0)
    {
        OLED_ShowCHinese(82,4,54);
        OLED_ShowCHinese(98,4,55);
    }

    OLED_ShowCHinese(2,4,32);
    OLED_ShowCHinese(18,4,33);  //����
    if(month==1||month==2)
    week = (day+2*(month+12)+3*(month+13)/5+(year-1)+(year-1)/4-(year-1)/100+(year-1)/400)%7;
    else if(month>12||day>31||month<=0||day<=0) week = 7;
    else week = (day+2*month+3*(month+1)/5+year+year/4-year/100+year/400)%7;
    switch(week)
    {
      case 7: OLED_ShowChar(34, 4, '*');
              break;
      case 0: OLED_ShowCHinese(34,4,34);
              break;
      case 1: OLED_ShowCHinese(34,4,35);
              break;
      case 2: OLED_ShowCHinese(34,4,36);
              break;
      case 3: OLED_ShowCHinese(34,4,37);
              break;
      case 4: OLED_ShowCHinese(34,4,38);
              break;
      case 5: OLED_ShowCHinese(34,4,39);
              break;
      case 6: OLED_ShowCHinese(34,4,40);
              break;
    }
    //ʱ��д��flash��
    flashtime[0] = hour/10;
    flashtime[1] = hour%10;
    flashtime[3] = min/10;
    flashtime[4] = min%10;
    flashtime[6] = second/10;
    flashtime[7] = second%10;
    write_flash_int(0x001800, flashtime, 8);
    write_flash_int(0x001880, timeset[1], 10);

}
void select_function(void)
{
    key_value=key();
    if(key_value!=0)
    {
        switch(key_value)
        {
          case 4:
          {
              settime();  //����ʱ��
              break;
          }
          case 8:
          {
              modex_flag=(modex_flag+1)%2;  //�л�ʱ��
              break;
          }
          case 12:
          {
              alarmclock_set();  //��������
              break;
          }
          case 16:
          {
              timer();  //��ʱ��
              break;
          }
          default: break;
        }
    }
}

void settime(void)
{
    pos_1:
    OLED_Clear();
    OLED_ShowCHinese(2,0,22);
    OLED_ShowCHinese(18,0,23);
    OLED_ShowCHinese(34,0,24);
    OLED_ShowCHinese(50,0,25);  //��ʱ�����á�
    OLED_ShowCHinese(4,2,22);
    OLED_ShowCHinese(20,2,23);
    OLED_ShowChar(36, 2, ':');  //��ʱ�䣺��
    OLED_ShowCHinese(2,4,26);
    OLED_ShowCHinese(18,4,27);
    OLED_ShowChar(34, 4, ':');  //�����ڣ���
    OLED_ShowChar(2, 6, 'A');
    OLED_ShowCHinese(10,6,28);
    OLED_ShowCHinese(26,6,29);  //��Aȷ����
    OLED_ShowChar(58, 6, 'B');
    OLED_ShowCHinese(66,6,30);
    OLED_ShowCHinese(82,6,31);  //��B��ա�
    if(modex_flag==1)
    {
        if(timeset_mod12_APflag==0) OLED_ShowString(106, 0, AM);
        else if(timeset_mod12_APflag==1) OLED_ShowString(106, 0, PM);
    }
    while(1)
    {
        _delay_cycles(600000);
         key_value=key();
                          if(key_value!=0)
                          {
                                if(i<8) {
                                  if(key_value>4&&key_value<8) timeset[0][i] = key_value-1;
                                  else if(key_value>8&&key_value<12) timeset[0][i] = key_value-2;
                                  else if(key_value==14) timeset[0][i] = 0;
                                  else timeset[0][i] = key_value;
                                }
                                else if(i>=8 && i<18) {
                                  if(key_value>4&&key_value<8) timeset[1][i-8] = key_value-1;
                                  else if(key_value>8&&key_value<12) timeset[1][i-8] = key_value-2;
                                  else if(key_value==14) timeset[1][i-8] = 0;
                                  else timeset[1][i-8] = key_value;
                                }
                                else if(i==18) i = 0;

                            switch(key_value)/*switch*/
                            {
                              case 1: if(i<8) OLED_ShowChar(52+(i)*8,2,'1');
                                      else if(i>=8) OLED_ShowChar(42+(i-8)*8,4,'1');
                                      i+=1;
                                      break;
                              case 2: if(i<8) OLED_ShowChar(52+(i)*8,2,'2');
                                      else if(i>=8) OLED_ShowChar(42+(i-8)*8,4,'2');
                                      i+=1;
                                      break;
                              case 3: if(i<8) OLED_ShowChar(52+(i)*8,2,'3');
                                      else if(i>=8) OLED_ShowChar(42+(i-8)*8,4,'3');
                                      i+=1;
                                      break;
                              case 5: if(i<8) OLED_ShowChar(52+(i)*8,2,'4');
                                      else if(i>=8) OLED_ShowChar(42+(i-8)*8,4,'4');
                                      i+=1;
                                      break;
                              case 6: if(i<8) OLED_ShowChar(52+(i)*8,2,'5');
                                      else if(i>=8) OLED_ShowChar(42+(i-8)*8,4,'5');
                                      i+=1;
                                      break;
                              case 7: if(i<8) OLED_ShowChar(52+(i)*8,2,'6');
                                      else if(i>=8) OLED_ShowChar(42+(i-8)*8,4,'6');
                                      i+=1;
                                      break;
                              case 9: if(i<8) OLED_ShowChar(52+(i)*8,2,'7');
                                      else if(i>=8) OLED_ShowChar(42+(i-8)*8,4,'7');
                                      i+=1;
                                      break;
                              case 10: if(i<8) OLED_ShowChar(52+(i)*8,2,'8');
                                       else if(i>=8) OLED_ShowChar(42+(i-8)*8,4,'8');
                                       i+=1;
                                       break;
                              case 11: if(i<8) OLED_ShowChar(52+(i)*8,2,'9');
                                       else if(i>=8) OLED_ShowChar(42+(i-8)*8,4,'9');
                                       i+=1;
                                       break;
                              case 13: if(i<8) OLED_ShowChar(52+(i)*8,2,'.');
                                       else if(i>=8) OLED_ShowChar(42+(i-8)*8,4,'.');
                                       i+=1;
                                       break;
                              case 14: if(i<8) OLED_ShowChar(52+(i)*8,2,'0');
                                       else if(i>=8) OLED_ShowChar(42+(i-8)*8,4,'0');
                                       i+=1;
                                       break;
                              case 15: if(i<8) OLED_ShowChar(52+(i)*8,2,'#');
                                       else if(i>=8) OLED_ShowChar(42+(i-8)*8,4,'#');
                                       i+=1;
                                       break;
                              //���
                              case 8: for(j=0;j<2;j++)
                                      {
                                          for(k=0;k<10;k++)
                                          {
                                              timeset[j][k] = 0;
                                          }
                                      }
                                      i = 0;
                                      OLED_Clear();
                                      goto pos_1;
                              //ȷ��
                              case 4: i = 0;
                                      goto pos_2;
                              //12ʱ�����л�AM��PM
                              case 12: if(modex_flag==1)
                                       {
                                          timeset_mod12_APflag = (timeset_mod12_APflag+1)%2;
                                          goto pos_1;
                                       }
                              default: break;
                            }
                        }
     }
     pos_2:
     if(modex_flag==0)
     {
       hour = 10*timeset[0][0] + timeset[0][1];
     }
     else if(modex_flag==1)  //12Сʱ�µ�ʱ������
     {
         if(timeset_mod12_APflag==0)
         {
             hour = 10*timeset[0][0] + timeset[0][1];
         }
         else if(timeset_mod12_APflag==1)
         {
             if((10*timeset[0][0] + timeset[0][1])==12) hour = 10*timeset[0][0] + timeset[0][1];
             else hour = 10*timeset[0][0] + timeset[0][1] + 12;
         }
     }
     min = 10*timeset[0][3] + timeset[0][4];
     second = 10*timeset[0][6] + timeset[0][7];

     year= 1000*timeset[1][0] + 100*timeset[1][1] + 10*timeset[1][2] + timeset[1][3];
     month = 10*timeset[1][5] + timeset[1][6];
     day = 10*timeset[1][8] + timeset[1][9];
     OLED_Clear();
}

void write_flash_int (unsigned int addr, int *array, int count)
{
    unsigned int *Flash_ptr;            // ����Flashָ��
    int i;

    // Flash�Ĳ���
    Flash_ptr = (unsigned int*)addr;    // ��ʼ��Flashָ��
    FCTL1 = FWKEY + ERASE;              // �����Flash�ν��в���
    FCTL3 = FWKEY;                      // ����(Lock = 0)
    *Flash_ptr = 0;                     // ����һ�ο�д����������

    // дFlash
    FCTL1 = FWKEY + WRT;                // ����д
    for(i = 0; i<count; i++)            // ��count��int��������д��ָ����ַ��
    {
        *Flash_ptr++ = array[i];   //ָ����һ������λ�ã�unsigned intΪ16bit
    }
    FCTL1 = FWKEY;                      // ���д����λ
    FCTL3 = FWKEY + LOCK;               // ����(Lock = 1)
}
void read_flash_int(unsigned int addr, int *array, int count)
{
    int *address = (int *)addr;
    int i;
    for(i = 0; i<count; i++)
    {
        array[i] = *address++;          // address+=2
    }
}
void read_timedata(void)
{
    read_flash_int(0x001800, timeset[0], 8);
    read_flash_int(0x001880, timeset[1], 10);
    if(timeset[0][0]!=-1||timeset[1][0]!=-1)
    {
      hour = 10*timeset[0][0] + timeset[0][1];
      min = 10*timeset[0][3] + timeset[0][4];
      second = 10*timeset[0][6] + timeset[0][7];

      year= 1000*timeset[1][0] + 100*timeset[1][1] + 10*timeset[1][2] + timeset[1][3];
      month = 10*timeset[1][5] + timeset[1][6];
      day = 10*timeset[1][8] + timeset[1][9];
    }
}
void alarmclock_set(void)
{
    pos_3:
    OLED_Clear();
    OLED_ShowCHinese(2,0,41);
    OLED_ShowCHinese(18,0,42);
    OLED_ShowCHinese(34,0,24);
    OLED_ShowCHinese(50,0,25);  //���������á�

    OLED_ShowCHinese(2,4,41);
    OLED_ShowCHinese(18,4,42);
    OLED_ShowChar(50, 4, ':');  //������*����

    if(alarm_flag[alarm_ex]==1) OLED_ShowString(10, 6, ON);
    else if(alarm_flag[alarm_ex]==0) OLED_ShowString(10, 6, OFF);
    if(alarm_ex==0)
    {
        OLED_ShowCHinese(34,4,34); //����һ
        OLED_ShowNum(58, 4, alarm_hour[0], 2, 16);
        OLED_ShowChar(74, 4, ':');
        OLED_ShowNum(82, 4, alarm_min[0], 2, 16);
        OLED_ShowChar(98, 4, ':');
        OLED_ShowNum(106, 4, alarm_second[0], 2, 16);
    }
    else if(alarm_ex==1)
    {
        OLED_ShowCHinese(34,4,35); //���Ӷ�
        OLED_ShowNum(58, 4, alarm_hour[1], 2, 16);
        OLED_ShowChar(74, 4, ':');
        OLED_ShowNum(82, 4, alarm_min[1], 2, 16);
        OLED_ShowChar(98, 4, ':');
        OLED_ShowNum(106, 4, alarm_second[1], 2, 16);
    }
    else if(alarm_ex==2)
    {
        OLED_ShowCHinese(34,4,36); //������
        OLED_ShowNum(58, 4, alarm_hour[2], 2, 16);
        OLED_ShowChar(74, 4, ':');
        OLED_ShowNum(82, 4, alarm_min[2], 2, 16);
        OLED_ShowChar(98, 4, ':');
        OLED_ShowNum(106, 4, alarm_second[2], 2, 16);
    }
    else OLED_ShowChar(34, 4, '*');

      while(1)
      {
          _delay_cycles(600000);
          key_value=key();
          if(key_value!=0)
          {
              if(u<8)
              {
                  if(key_value>4&&key_value<8) alarmset[alarm_ex][u] = key_value-1;
                  else if(key_value>8&&key_value<12) alarmset[alarm_ex][u] = key_value-2;
                  else if(key_value==14) alarmset[alarm_ex][u] = 0;
                  else alarmset[alarm_ex][u] = key_value;
              }
              if(u==8) u=0;
              switch(key_value)
              {
                case 1: if(u<8) OLED_ShowChar(2+u*8,2,'1');
                        u+=1;
                        break;
                case 2: if(u<8) OLED_ShowChar(2+u*8,2,'2');
                        u+=1;
                        break;
                case 3: if(u<8) OLED_ShowChar(2+u*8,2,'3');
                        u+=1;
                        break;
                case 5: if(u<8) OLED_ShowChar(2+u*8,2,'4');
                        u+=1;
                        break;
                case 6: if(u<8) OLED_ShowChar(2+u*8,2,'5');
                        u+=1;
                        break;
                case 7: if(u<8) OLED_ShowChar(2+u*8,2,'6');
                        u+=1;
                        break;
                case 9: if(u<8) OLED_ShowChar(2+u*8,2,'7');
                        u+=1;
                        break;
                case 10: if(u<8) OLED_ShowChar(2+u*8,2,'8');
                         u+=1;
                         break;
                case 11: if(u<8) OLED_ShowChar(2+u*8,2,'9');
                         u+=1;
                         break;
                case 13: if(u<8) OLED_ShowChar(2+u*8,2,'.');
                         u+=1;
                         break;
                case 14: if(u<8) OLED_ShowChar(2+u*8,2,'0');
                         u+=1;
                         break;
                //#ȷ��
                case 15: alarm_hour[alarm_ex] = alarmset[alarm_ex][0]*10 + alarmset[alarm_ex][1];
                         alarm_min[alarm_ex] = alarmset[alarm_ex][3]*10 + alarmset[alarm_ex][4];
                         alarm_second[alarm_ex] = alarmset[alarm_ex][6]*10 + alarmset[alarm_ex][7];
                         u = 0;
                         goto pos_3;
                //A�л���������һ������
                case 4: alarm_ex = (alarm_ex+1)%3;
                        u = 0;
                        goto pos_3;
                //B����ʹ��
                case 8: alarm_flag[alarm_ex] = (alarm_flag[alarm_ex]+1)%2;
                        goto pos_3;
                //C���
                case 12: for(j=0;j<8;j++)
                         {
                           alarmset[alarm_ex][j] = 0;
                         }
                         u = 0;
                         goto pos_3;
                //D�˳�
                case 16: u = 0;
                         OLED_Clear();
                         goto pos_4;
                default: break;
              }
          }
      }
      pos_4:
      P5DIR |= BIT1;
      P3DIR |= BIT3 + BIT4 + BIT5 + BIT6 + BIT7;
      P5OUT |= BIT1;
      P3OUT |= BIT3 + BIT4 + BIT5 + BIT6 + BIT7;  //���ӵ�����˸�Ĺܽ�����Ϊ�ߣ�����
}
void alarmclock(void)
{
    for(j=0;j<3;j++)
    {
        if(alarm_flag[j]!=0 && second==alarm_second[j] && min==alarm_min[j] && hour==alarm_hour[j]) alarm_light_enable=1;
    }
       if(alarm_light_timecount!=10 && alarm_light_enable==1)
       {
          P5OUT ^= BIT1;
          P3OUT ^= BIT3 + BIT4 + BIT5 + BIT6 + BIT7;
          alarm_light_timecount++;
       }
       else if(alarm_light_timecount==10)
       {
          P5OUT |= BIT1;
          P3OUT |= BIT3 + BIT4 + BIT5 + BIT6 + BIT7;
          alarm_light_enable = 0;
          alarm_light_timecount = 0;
       }
       //����ʱ�䵽����LED��P2��3��4��5��6��7��˸���
}

void timer(void)
{
    OLED_Clear();
    OLED_ShowCHinese(2,0,43);
    OLED_ShowCHinese(18,0,22);
    OLED_ShowCHinese(34,0,44);  //��ʱ��

    OLED_ShowChar(2, 4, 'A');
    OLED_ShowCHinese(10,4,45);
    OLED_ShowCHinese(26,4,46);  //A��ʼ
     OLED_ShowChar(50, 4, 'B');
     OLED_ShowCHinese(58,4,47);
     OLED_ShowCHinese(74,4,48);  //B��ͣ
      OLED_ShowChar(2, 6, 'C');
      OLED_ShowCHinese(10,6,49);
      OLED_ShowCHinese(26,6,50);  //C���
       OLED_ShowChar(50, 6, 'D');
       OLED_ShowCHinese(58,6,51);
       OLED_ShowCHinese(74,6,52);  //D�˳�

    while(1)
    {
        _delay_cycles(600000);
        key_value=key();
        OLED_ShowNum(2, 2, timer_hour, 2, 16);
        OLED_ShowChar(18, 2, ':');
        OLED_ShowNum(26, 2, timer_min, 2, 16);
        OLED_ShowChar(42, 2, ':');
        OLED_ShowNum(50, 2, timer_second, 2, 16);
        if(key_value!=0)
        {
            switch(key_value)
            {
               case 4: timer_enable = 1;  //A��ʼ
                       break;
               case 8: timer_enable = 0;  //B��ͣ
                       break;
               case 12: timer_hour = 0;   //C���
                        timer_min = 0;
                        timer_second = 0;
                        break;
               case 16: OLED_Clear();  //D�˳�
                        goto pos_5;
               default: break;
            }
        }
    }
    pos_5:
}

void temp_set(void)
{
    REFCTL0 &= ~REFMSTR;                      // Reset REFMSTR to hand over control to
                                                // ADC12_A ref control registers
    ADC12CTL0 = ADC12SHT0_8 + ADC12REFON + ADC12ON;
                                                // ���òο���ѹΪ1.5V����AD
    ADC12CTL1 = ADC12SHP;                     // ����������������Ϊ�ڲ���ʱ��
    ADC12MCTL0 = ADC12SREF_1 + ADC12INCH_10;  // AD��A10ͨ���������¶ȴ��������
    ADC12IE = 0x001;                          // ADC_IFG upon conv result-ADCMEMO
    __delay_cycles(100);                      // Allow ~100us (at default UCS settings)
                                                // for REF to settle
    ADC12CTL0 |= ADC12ENC;                    // ʹ��AD
}
void temp_show(void)
{
    ADC12CTL0 &= ~ADC12SC;
        ADC12CTL0 |= ADC12SC;                   // ��ʼ����

        __bis_SR_register(LPM4_bits + GIE);     // LPM0 with interrupts enabled
        __no_operation();

        // Temperature in Celsius. See the Device Descriptor Table section in the
        // System Resets, Interrupts, and Operating Modes, System Control Module
        // chapter in the device user's guide for background information on the
        // used formula.
        temperatureDegC = (float)(((long)temp - CALADC12_15V_30C) * (85 - 30)) /
                (CALADC12_15V_85C - CALADC12_15V_30C) + 30.0f; //���϶Ȼ���

        temtemp = temperatureDegC*100;
        OLED_ShowString(2,6,"Temp:");
        OLED_ShowString(58,6,".");
        OLED_ShowNum(42,6,temtemp/100,2,16);
        OLED_ShowNum(66,6,temtemp%100,2,16);    //�¶���ʾ

        // Temperature in Fahrenheit Tf = (9/5)*Tc + 32
        temperatureDegF = temperatureDegC * 9.0f / 5.0f + 32.0f;//���϶Ȼ���
        _delay_cycles(100000);
        __no_operation();                       // SET BREAKPOINT HERE
}
#pragma vector=ADC12_VECTOR
__interrupt void ADC12ISR (void)
{
  switch(__even_in_range(ADC12IV,34))
  {
  case  0: break;                           // Vector  0:  No interrupt
  case  2: break;                           // Vector  2:  ADC overflow
  case  4: break;                           // Vector  4:  ADC timing overflow
  case  6:                                  // Vector  6:  ADC12IFG0
    temp = ADC12MEM0;                       // ��ȡ������жϱ�־�ѱ����
    __bic_SR_register_on_exit(LPM4_bits);   // Exit active CPU
  case  8: break;                           // Vector  8:  ADC12IFG1
  case 10: break;                           // Vector 10:  ADC12IFG2
  case 12: break;                           // Vector 12:  ADC12IFG3
  case 14: break;                           // Vector 14:  ADC12IFG4
  case 16: break;                           // Vector 16:  ADC12IFG5
  case 18: break;                           // Vector 18:  ADC12IFG6
  case 20: break;                           // Vector 20:  ADC12IFG7
  case 22: break;                           // Vector 22:  ADC12IFG8
  case 24: break;                           // Vector 24:  ADC12IFG9
  case 26: break;                           // Vector 26:  ADC12IFG10
  case 28: break;                           // Vector 28:  ADC12IFG11
  case 30: break;                           // Vector 30:  ADC12IFG12
  case 32: break;                           // Vector 32:  ADC12IFG13
  case 34: break;                           // Vector 34:  ADC12IFG14
  default: break;
  }
}
