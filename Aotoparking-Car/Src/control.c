#include "control.h"
#include "tim.h"
#include "main.h"
#include "gpio.h"
#define T 0.156f
#define L 0.1445f
#define K 622.8f
#define __HAL_TIM_SET_PULSE1(__HANDLE__, __COUNTER__)  ((__HANDLE__)->Instance->CCR1 = (__COUNTER__))
#define __HAL_TIM_SET_PULSE2(__HANDLE__, __COUNTER__)  ((__HANDLE__)->Instance->CCR2 = (__COUNTER__))
int Flag_Target;
int CaptureNumber=0;    //�����������ȡֵ
int TurnNumberA=0;   //��¼ת������Ȧ��
int TurnNumberB;
int Encoder_Left,Encoder_Right;             //���ұ��������������
int Motor_A,Motor_B,Servo,Target_A,Target_B;  //���PWM���� 
float Velocity_KP=10,Velocity_KI=1;	  

/**************************************************************************
�������ܣ�������PWM�����ʼ��
��ڲ�������
����  ֵ����
**************************************************************************/
void PWM_Init(void)
{
	HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim3,TIM_CHANNEL_2);
	HAL_TIM_PWM_Start(&htim4,TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim4,TIM_CHANNEL_2);	
}
/**************************************************************************
�������ܣ����������ܳ�ʼ��
��ڲ�������
����  ֵ����
**************************************************************************/
void Encoder_Init(void)
{
	HAL_TIM_Encoder_Start(&htim1,TIM_CHANNEL_1);
	HAL_TIM_Encoder_Start(&htim1,TIM_CHANNEL_2);
	HAL_TIM_Encoder_Start(&htim2,TIM_CHANNEL_1);
	HAL_TIM_Encoder_Start(&htim2,TIM_CHANNEL_2);
}

/**************************************************************************
�������ܣ�С���˶���ѧģ��
��ڲ������ٶȺ�ת��
����  ֵ����
**************************************************************************/
void Kinematic_Analysis(float velocity,float angle)
{
		Target_A=velocity*(1+T*tan(angle)/2/L); 
		Target_B=velocity*(1-T*tan(angle)/2/L);      //���ֲ���
		Servo=1500+angle*K;                    //���ת��   
}


/**************************************************************************
�������ܣ�����PI������
��ڲ���������������ֵ��Ŀ���ٶ�
����  ֵ�����PWM
��������ʽ��ɢPID��ʽ 
pwm+=Kp[e��k��-e(k-1)]+Ki*e(k)+Kd[e(k)-2e(k-1)+e(k-2)]
e(k)������ƫ�� 
e(k-1)������һ�ε�ƫ��  �Դ����� 
pwm�����������
�����ǵ��ٶȿ��Ʊջ�ϵͳ���棬ֻʹ��PI����
pwm+=Kp[e��k��-e(k-1)]+Ki*e(k)
**************************************************************************/
int Incremental_PI_A (int Encoder,int Target)
{ 	
	 static int Bias,Pwm,Last_bias;
	 Bias=Target-Encoder;                //����ƫ��
	 Pwm+=Velocity_KP*(Bias-Last_bias)+Velocity_KI*Bias;   //����ʽPI������
	 Last_bias=Bias;	                   //������һ��ƫ�� 
	 return Target;                         //�������
}
int Incremental_PI_B (int Encoder,int Target)
{ 	

	 static int Bias,Pwm,Last_bias;
	 Bias=Target-Encoder;                //����ƫ��
	 Pwm+=Velocity_KP*(Bias-Last_bias)+Velocity_KI*Bias;   //����ʽPI������
	 Last_bias=Bias;	                   //������һ��ƫ�� 
	 return Target;                         //�������
}

/**************************************************************************
�������ܣ�����ֵ����
��ڲ�����int
����  ֵ��unsigned int
**************************************************************************/
int myabs(int a)
{ 		   
	  int temp;
		if(a<0)  temp=-a;  
	  else temp=a;
	  return temp;
}
/**************************************************************************
�������ܣ�����PWM��ֵ 
��ڲ�������
����  ֵ����
**************************************************************************/
void Xianfu_Pwm(void)
{	
	  int Amplitude=800;    //===PWM������100 ������90
    if(Motor_A<-Amplitude) Motor_A=-Amplitude;	
		if(Motor_A>Amplitude)  Motor_A=Amplitude;	
	  if(Motor_B<-Amplitude) Motor_B=-Amplitude;	
		if(Motor_B>Amplitude)  Motor_B=Amplitude;		
		if(Servo<(1500-500))     Servo=1500-500;	  //����޷�
		if(Servo>(1500+500))     Servo=1500+500;		  //����޷�
}
/**************************************************************************
�������ܣ���ֵ��PWM�Ĵ���
��ڲ���������PWM������PWM
����  ֵ����
**************************************************************************/
void Set_Pwm(int motor_a,int motor_b,int servo)
{
		__HAL_TIM_SET_PULSE1(&htim3,motor_a);
		__HAL_TIM_SET_PULSE2(&htim3,motor_b);
		__HAL_TIM_SET_PULSE1(&htim4,servo);
}

int Read_Encoder(int TIMx)
{
	switch(TIMx)
	{
		case 1: 
			CaptureNumber=-(__HAL_TIM_GET_COUNTER(&htim1)-10000);
			__HAL_TIM_SET_COUNTER(&htim1, 10000);
			break;
//		case 2: 	
//			CaptureNumber=__HAL_TIM_GET_COUNTER(&htim2)-10000;
//			__HAL_TIM_SET_COUNTER(&htim2, 10000);
//			break;		
	}
	
	return CaptureNumber;  // ���ص��ǵ�λʱ��������ת����Ȧ��
}
	
void Control(float Velocity,float Angle)
{
		HAL_GPIO_WritePin(STBY_GPIO_Port,STBY_Pin,GPIO_PIN_SET);
		TurnNumberA+=Read_Encoder(1);
//	  Encoder_Right=Read_Encoder(2);   																	//===��ȡ��������ֵ
	
	if(Velocity>0)
	{
		HAL_GPIO_WritePin(AIN1_GPIO_Port,AIN1_Pin,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(AIN2_GPIO_Port,AIN2_Pin,GPIO_PIN_SET);
		HAL_GPIO_WritePin(BIN1_GPIO_Port,BIN1_Pin,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(BIN2_GPIO_Port,BIN2_Pin,GPIO_PIN_SET);		
		Kinematic_Analysis(Velocity,Angle);     														//С���˶�ѧ����   
		Motor_A=Incremental_PI_A(Encoder_Left,Target_A);                   //===�ٶȱջ����Ƽ�����A����PWM
		Motor_B=Incremental_PI_B(Encoder_Right,Target_B);                  //===�ٶȱջ����Ƽ�����B����PWM 
		Xianfu_Pwm();                                                      //===PWM�޷�
		Set_Pwm(Motor_A,Motor_B,Servo);   																	//===��ֵ��PWM�Ĵ���  
	}		   
	else	
	{	
		HAL_GPIO_WritePin(AIN1_GPIO_Port,AIN1_Pin,GPIO_PIN_SET);
		HAL_GPIO_WritePin(AIN2_GPIO_Port,AIN2_Pin,GPIO_PIN_RESET);
		HAL_GPIO_WritePin(BIN1_GPIO_Port,BIN1_Pin,GPIO_PIN_SET);
		HAL_GPIO_WritePin(BIN2_GPIO_Port,BIN2_Pin,GPIO_PIN_RESET);		
		Kinematic_Analysis(-Velocity,Angle);     														//С���˶�ѧ����   
		Motor_A=Incremental_PI_A(Encoder_Left,Target_A);                   //===�ٶȱջ����Ƽ�����A����PWM
		Motor_B=Incremental_PI_B(Encoder_Right,Target_B);                  //===�ٶȱջ����Ƽ�����B����PWM 
		Xianfu_Pwm();                                                      //===PWM�޷�
		Set_Pwm(Motor_A,Motor_B,Servo);   																	//===��ֵ��PWM�Ĵ���  
	}		
}


void Move(float Velocity,float Angle,float TurnNumber)
{
	if(TurnNumber>0)
	{
	while((TurnNumber*390*4-TurnNumberA)>0)
	{
	Control(Velocity,Angle);
	HAL_Delay(10);
	}
	Control(0,0);
	TurnNumberA=0;
	}
	else
	{
	while((TurnNumber*390*4-TurnNumberA)<0)
	{
	Control(Velocity,Angle);
	HAL_Delay(10);
	}
	Control(0,0);
	TurnNumberA=0;
	}
}

void OutDoor(void)
{
		Move(200,0,2.5);
}

void ParkCenter()
{
	Move(200,-PI*50/180,0.98);
	Move(200,0,0.7);
}

void Park1(void)
{
	Move(-200,0,-0.2);
	Move(-200,PI*50/180,-2.8);
	Move(-200,0,-1);
}

void Park2(void)
{
	Move(-200,0,-1.9);
	Move(200,PI*50/180,3.0);
}

void Park3(void)
{
	Move(-200,0,-0.6);
	Move(200,PI*50/180,3.10);
}

void Park4(void)
{
	Move(-200,0,-1.95);
	Move(200,-PI*48/180,0.8);
	Move(200,PI*60/180,1.9);
	Move(-100,0,-0.5);
}
void Park4_1(void)
{
	Move(-200,0,-1.95);
	Move(200,-PI*48/180,0.7);
	Move(200,PI*60/180,1.5);
	Move(-100,0,-0.3);
}
void Back1(void)
{
	Move(200,0,4);
}
void Back2(void)
{
	Move(-200,-PI*50/180,-1.10);
	Move(200,0,0.5);
	Move(200,PI*50/180,3.1);
}

void Back3(void)
{
	Move(-200,PI*50/180,-3.3);
	Move(200,0,0.5);
	Move(-200,-PI*50/180,-1.0);
	Move(-200,0,-1.0);
	
}
void Look(int x)
{
	switch(x)
	{
		case 0:
			__HAL_TIM_SET_PULSE2(&htim4,500);
			break;
		case 1:
			__HAL_TIM_SET_PULSE2(&htim4,2300);
			break;
		case 2:
			__HAL_TIM_SET_PULSE2(&htim4,1500);
			
	}
}
