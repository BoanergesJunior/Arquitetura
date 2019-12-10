#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <math.h>

using namespace std;

#define MAX_FILE_SIZE 65536 //64KB
#define LINE_SIZE 4

unsigned int memory[MAX_FILE_SIZE / 4];//16K int
unsigned short fileSize;

unsigned long pc = 0x00000040, sp = (MAX_FILE_SIZE/4) - 1;
unsigned int IR = 0;

unsigned long XW[31]={}; //31 registradores de 64 bits. Se X, usa os 64 bits (long). Se W, usa os 32 bits da direita (int) e o bit mais significativo (31) é repetido para a metade da esquerda (de 32 a 63).  B1.2.1 pág. 81
float S[32] = {}; //reg float

unsigned long regA, regB, *regD;
float fRegA, fRegB,*fRegD, *fRegT;

signed long result; 
float fResult;

int memCode = 0;
bool wb = 0, neg = 0, zero = 0, overflow = 0;//memcode 1 e 2 leitura e escrita 32 bits 3 e 4 leitura e escrita 64 bits é leitura? memcode = 1 é escrita? memcode = 2 é 64 bits? memcode += 2
bool sair = false;
bool _float = false;


enum execcodes {ADD, SUB, MOV, RIGHT, LEFT, NONE, fDIV, fNEG, fSTR, fLDR, fMUL, fADD, fSUB, fMOV} execcode;


ofstream arquivo("LOG.txt");    //Arquivo de saída

//NOTAS:
//regA = read data1
//regB = read data2
//*regD = registrador destino
//XW vetor de registradores
//execcodes enumera os tipos de operações realizáveis na ULA

//Carrega arquivo binario na memoria
void loadBinary(const char *filename) {
	//Open the file for reading in binary mode
	FILE *fIn = fopen(filename, "rb");
	long position;

	if (fIn != NULL) {
		//Go to the end of the file
		const int fseek_end_value = fseek(fIn, 0, SEEK_END);
		if (fseek_end_value != -1) {

			//Get the current position in the file (in bytes)
			position = ftell(fIn);
			if (position != -1) {

				//Go back to the beginning of the file
				const int fseek_set_value = fseek(fIn, 0, SEEK_SET);

				//If error, exit
				if (fseek_set_value == -1) {
					printf("Error reading file.");
					exit(1);
				}

				//If file too big, exit
				if (fseek_set_value > MAX_FILE_SIZE) {
					printf("Maximum allowed file size is 64KB.");
					exit(1);
				}

				//Read the whole file to memory
				fileSize = fread(memory, 1, position, fIn);
			}
		}
		fclose(fIn);
	}
}

//Escreve arquivo binario em um arquivo legível
void writeFileAsText (const char *basename) {
	char filename[256] = "txt_";
	FILE * fp;
	int i,j;

	strcat(filename,basename);
	strcat(filename,".txt");

	fp = fopen (filename, "w+");

	//legenda
	fprintf(fp, "ADDR    ");
	for (j=0; j<LINE_SIZE; j++) {
		fprintf(fp, "ADDR+%02X  ", 4*j);
	}
	fprintf(fp, "\n----------------------------------------------------------------------------\n");


	//programa
	i=0;
	for (i = 0; i < fileSize / 4; i+=LINE_SIZE) {
		fprintf(fp, "%04X    ", 4*i);
		for (j=0; j<LINE_SIZE; j++) {
			fprintf(fp, "%08X ", memory[i+j]);
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
}


//Busca a instrução na memória
void instructionFetch(){                                                
	IR = memory[pc>>2];//pc>>2 divide o numero binario por 4
	arquivo << "ri " << std::hex << (unsigned int) pc << endl;
}


float exec8to32(unsigned int imm8){
	unsigned int mantissa,exp,sign;
	float imm;
	
	mantissa = imm8 & (0x0000000F); // acha mantissa 
	exp = (imm8 & (0x00000070)) >> 4;
	sign = imm8 >> 7;
	//cout << "exp antes " << exp << endl;
	if ((exp & 0x00000004) > 0) { // troca o sinal por conta da formula(primeiro bit)
		exp = (exp & 0xFFFFFFFB);
	} else {
		exp = exp | 0x00000004;
	}
	exp=exp-3;

	imm =((float) (16 + mantissa) / 16);
	//cout << "imm" << imm << endl;		

	//imm = (((-1)^sign) * (2^exp) * (imm));// imm tem que ser float seguindo funcao do livro
	imm = ((pow(-1,sign)*pow(2,exp)*imm));
	//cout << "exp " << 2^exp << endl;		

	//cout << "a" << imm8 << endl;
	//cout << imm << endl;
	return imm;
/*codigo compartilhado do grupo:
*Wallison
*Natanael Sindou
*Tales Ribeiro
*Rafael Garcia
*/
}


/*Decodifica a instrução, e informa a operação a ser feita na ULA por meio da
variável execcode*/
void instructionDecode (){
	cout << "SP " << sp << endl << flush;
	int n, d, m, t;
	//alterar a decodificacao para identificar se e integer ou float
	//fdiv =  1E201821  c7.2.91 pag 1466
	//fneg = 1E214001  c7.2.133 pag 1559
	//fadd = 1E202800  c7.2.43  pag 1346
	//fmov = 1E229000 c7.2.125 pag 1538
	//fsub = 1E203820 c7.2.159 pag 1615
	//FMUL = 1E200821 c7.2.129 pag 1548
	//LRD = BD4013E0 c7.2.176 pag 1668
	//STR = BC217800 c7.2.315 pag 1982
	
	switch(IR & 0xFF20FC00){//mascara p/ op bit a bit
		//certo
		case 0x1E201800: //FDIV (scalar)= 1E201821  c7.2.91 pag 1466 
			d = (IR & 0x0000001F);
			fRegD = &S[d];
			
			n = (IR & 0x000003E0) >> 5; // indice do reg a da op
			fRegA = S[n];
			
			m = (IR & 0x001F0000) >> 16;
			fRegB = S[m];// reg b
			
			execcode = fDIV; //flags
			memCode = 0; //verificar
			wb = 1;//verificar
			_float = true;
			cout << "div" << endl << flush;
			break;
		default:
			//cout << "Não foi implementado";
			break;
	}
	
	//certo
	switch(IR & 0xFF3FFC00){//FNEG(scalar)
		case 0x1E214000:
			d = (IR & 0x0000001F);
			fRegD = &S[d];
			
			n = (IR & 0x000003E0) >> 5; // indice do reg a da op
			fRegA = S[n];
			
			execcode = fNEG; //flags
			memCode = 0; //verificar
			wb = 1; //verificar
			_float = true;
			break;
			cout << "neg" << endl << flush;
		default:
			//cout << "Não foi implementado";
			break;
	}
	
	//certo
	switch(IR & 0xF720FC00){//FADD(scalar)
		case 0x1E202800:
			d = (IR & 0x0000001F);
			fRegD = &S[d];
			
			n = (IR & 0x000003E0) >> 5;
			fRegA = S[n];

			m = (IR & 0x001F0000) >> 16; 
			fRegB = S[m];
			
			execcode = fADD;
			memCode = 0;
			wb = 1;
			_float = true;
			cout << "add" << endl << flush;
			break;
		default:
			//cout << "Não foi implementado";
			break;
	}

	//certo
	switch(IR & 0xFF201FE0){ // para 10 bits iniciais fixos,onde ele tem valores fixos no livro
		case 0x1E201000: //FMOV ve onde tem um no livro e mantem 1 e se tiver 0 muda pra 0  	
			unsigned int imm8;
			d = (IR & 0x0000001F);
			imm8 =( IR & (0x001FE000)) >> 13;
			//cout << imm8 << "decodificacao" << endl;
			//a partir daqui mandar pro estagio de execucao e salvar o imm8 para o regA
			regA = imm8;
			execcode = fMOV;
			memCode = 0;
			wb = 1;//verificar
			_float = true;
			cout << "mov" << endl << flush;
			break;
		default:
			//cout << "Não foi implementado";
			break;
	}
	
	//certo
	switch(IR & 0xFF20FC00){//FSUB(scalar)
		case 0x1E203800:
			d = (IR & 0x0000001F);
			fRegD = &S[d];
			
			n = (IR & 0x000003E0) >> 5;
			fRegA = S[n];

			m = (IR & 0x001F0000) >> 16; 
			fRegB = S[m];
			
			execcode = fSUB;
			memCode = 0;
			wb = 1; //verificar
			_float = true;
			cout << "sub" << endl << flush;
			break;
			
		default:
			//cout << "Não foi implementado";
			break;
	}
	
	
	switch(IR & 0xFF20FC00){//FMUL
		case 0x1E200800:
			d = (IR & 0x0000001F);
			fRegD = &S[d];
			
			n = (IR & 0x000003E0) >> 5;
			fRegA = S[n];
			
			m = (IR & 0x001F0000) >> 16;
			fRegB = S[m];
			
			execcode = fMUL; //flags
			memCode = 0;//verificar
			wb = 1; //verificar
			_float = true;
			cout << "mul" << endl << flush;
			break;
		default:
			//cout << "Não foi implementado";
			break;
	}
	
	
	switch(IR & 0x3F400000){//STR (unsigned)
		case 0x3D000000:
			t = (IR & 0x0000001F);
			fRegD = &S[t];
			n = (IR & 0x000003E0) >> 5;
			/** pedro alterou aqui**/
			if (n == 31) {
				regA = sp;
			}
			else {
				regA = XW[n];
			}
			m = ((IR & 0x003FFC00) >> 10) << 2;
			regB = m;
			execcode = fSTR; //flags
			memCode = 7;//Store 32 Float
			wb = 0; //verificar
			_float = true;
			cout << "str" << endl << flush;
			break;
		// verificar proximo case wzr com 3 bits
		default:
			//cout << "Não foi implementado";
			break;
	}
	
	//certo
	switch(IR & 0x3F400000){//LDR(UNSIGNED)
		case 0x3D400000:
			//LDR = BD4013E0 c7.2.176 pag 1668
			t = (IR & 0x0000001F);
			fRegD = &S[t];

			n = (IR & 0x000003E0) >> 5;
			if (n == 31) {
				regA = sp;
			}
			else {
				regA = XW[n];
			}

			regB = ((IR & 0x003FFC00) >> 10) << 2;
			
			
			execcode = fLDR; //flags
			memCode = 6;// READ 32 FLOAT
			wb = 1; //verificar
			_float = true;
			cout << "ldr" << endl << flush;
			break;
		default:
			//cout << "Não foi implementado";
			break;
	}

	
	switch (IR & 0xFFC00000) { // op and bit a bit
		case 0x11000000://ADD C6.2.4 686 Immediate
			d = IR & 0x0000001F; // qual o endereço do reg d destino
			if(d == 31){ // 31, saiu do limite, vai pra sp
				regD = &sp;
			}else{ //se nao pega no vetor
				regD = &XW[d];
			}
			n = (IR & 0x000003E0) >> 5; // indice do reg a da op
			if(n == 31){ // igual a d
				regA = sp;
			}else{
				regA = XW[n];
			}
			regB = (IR & 0x003FFC00) >> 10; // reg b, imediato
			execcode = ADD; //flags
			memCode = 0;
			wb = 1;
			_float = false;
			break;
			
		case 0x91000000:
			d = IR & 0x0000001F; // qual o endereço do reg d destino
			if(d == 31){ // 31, saiu do limite, vai pra sp
				regD = &sp;
			}else{ //se nao pega no vetor
				regD = &XW[d];
			}
			n = (IR & 0x000003E0) >> 5; // indice do reg a da op
			if(n == 31){ // igual a d
				regA = sp;
			}else{
				regA = XW[n];
			}
			regB = (IR & 0x003FFC00) >> 10; // reg b, imediato
			execcode = ADD; //flags
			memCode = 0;
			wb = 1;
			_float = false;
			break;

		case 0xB9400000://LDR C6.2.119 Immediate (Unsigned offset) 886 
			//32 bits
			n = (IR & 0x000003E0) >> 5;
			if (n == 31) {
				regA = sp;
			}
			else {
				regA = XW[n];
			}
			regB = ((IR & 0x003FFC00) >> 10) << 2;
			
			d = IR & 0x0000001F;
			if (d == 31){
				regD = &sp;
			}else{
				regD = &XW[d];
			}
			execcode = ADD;
			memCode = 5;
			wb = 1;
			_float = false;
			break;

		case 0xB9800000://LDRSW C6.2.131 Immediate (Unsigned offset) 913
			// como é escrita em 64 bits, não há problema em decodificar
			n = (IR & 0x000003e0) >> 5;
			if (n == 31){
				regA = sp;
			}
			else {
				regA = XW[n];
			}
			regB = (IR & 0x003ffc00) >> 8; // immediate
			regD = &XW[IR & 0x0000001F];
			execcode = ADD;
			memCode = 1;
			wb = 1;
			_float = false;
			break;

		case 0xB9000000://STR C6.2.257 Unsigned offset 1135
			//size = 10, 32 bit
			n = (IR & 0x000003E0) >> 5;
			if(n == 31){
				regA = sp;
			}else{
				regA = XW[n];
			}
			regB = ((IR & 0x003FFC00) >> 10) << 2; //offset = imm12 << scale. scale == size
			d = IR & 0x0000001F;
			if (d == 31){
				regD = &sp;
			}else{
				regD = &XW[d];
			}
			execcode = ADD;
			memCode = 2; 
			wb = 0;
			_float = false;
			break;

		case 0xD1000000://SUB C6.2.289 Immediate 1199
			d = IR & 0x0000001F;//0000 0000 0000 0000 0000 0000 0001 1111 = 0x0000001F pois o registrador de destino está nos bits de 0 a 4
			if(d == 31){
				regD = &sp;
			}else{
				regD = &XW[d];
			}
			n = (IR & 0x000003E0) >> 5;//0000 0000 0000 0000 0000 0000 0011 1110 0000 = 0x000003E0 pois o registrador usado está nos bits de 5 a 9
			if(n == 31){
				regA = sp;
			}else{
				regA = XW[n];
			}
			//extrair o imediato e colocar no regB
			//o valor imediato é unsigned
			regB = (IR & 0x003FFC00) >> 10; //0000 0000 0011 1111 1111 1100 0000 0000 = 0x003FFC00 pois o imediato está entre nos bits de 10 a 21
			execcode = SUB;
			memCode = 0; //não escreve nem lê na memória
			wb = 1; //tem writeback
			_float = false;
			break;
	}
	
	switch (IR & 0xFF000000) {
		case 0x90000000://ADRP C6.2.10 698
			regA = ((unsigned long)(IR & 0x00FFFFE0) << 9) | ((unsigned long)(IR & 0x60000000) >> 17); //pega o imediato
			regB = (unsigned long)pc & 0xFFFFFFFFFFFFF000;
			d = IR & 0x0000001F;
			regD = &XW[d];
			
			execcode = ADD;
			memCode = 0;
			wb = 1;
			_float = false;
			break;
	}
	
	switch (IR & 0xFC000000) {
		case 0x14000000://B C6.2.24 722
			regA = pc;
			regB = ((IR & 0x03FFFFFF) << 2) - 4;
			regD = &pc;
			execcode = ADD;// PC = PC[] + offset
			memCode = 0;
			wb = 1;
			_float = false;
			break;
	}
	
	switch (IR & 0xFFE0FC00) {
		//1111 1111 1110 0000 1111 1100 0000 0000
		case 0xB8607800://LDR (Register) C6.2.121 891
			n = (IR & 0x000003E0) >> 5;
			if(n == 31){
				regA = sp;
			}else{
				regA = XW[n];
			}
			n = (IR & 0x001F0000) >> 16;
			if(n == 31){
				regB = sp << 2;
			}else{
				regB = XW[n] << 2;// como eu considero no and as "variaveis" como 1, so vai entrar nesse case se for: size 10, option 011 e s 1
			}
			d = IR & 0x0000001F;
			regD = &XW[d];
			execcode = ADD;
			memCode = 3;
			wb = 1;
			_float = false;
			break;
	}
	
	switch (IR & 0xFFE00000) {
		case 0x0B000000://ADD shifted register
			long shift = (IR & 0x00C00000) >> 21; //pega o valor do shift a ser efetuado no segundo operando (Rm)
			d = IR & 0x0000001F;
			if(d == 31){
				regD = &sp;
			}else{
				regD = &XW[d];
			}
			n = (IR & 0x000003E0) >> 5;
			regA = XW[n]; //Rn
			n = (IR & 0x001F0000) >> 15;
			regB = XW[n] << shift; //Rm      
			execcode = ADD;
			memCode = 0;
			wb = 1;
			_float = false;
			break;
	}
	switch (IR & 0xFFC0001F) {
		case 0x7100001F://CMP C6.2.57 Immediate 778
			//  nesse case
			//  sf == 0
			//  sh == 0
			n = (IR & 0x000003E0) >> 5;
			if (n == 31){
				regA = sp;
			}
			else {
				regA = XW[n];
			}
			regB = (IR & 0x003FFC00) >> 10;

			execcode = SUB; 
			memCode = 0; 
			wb = 0; 
			_float = false;
			break;
	}

	switch (IR & 0xFF00001F) {
		case 0x5400000D://B.cond C6.2.23 BLE 721
			regA = pc - 4;  
			regB = (unsigned long) (((signed long)((IR & 0x00ffffe0) << 8)) >> 11); 
			regD = &pc;
			execcode = NONE;
			wb = 0;
			if (zero || neg!=overflow){
				execcode = ADD;
				wb = 1;
			}
			memCode = 0;
			_float = false;
			break;
	}
	if (IR == 0xD503201F) { // nop pag 1028
		execcode = NONE;
		memCode = 0;
		wb = 0;
	}
	switch (IR & 0xFFFFFC1F) {
		case 0xD65F0000://RET C6.2.207 1053                                
			n = (IR & 0x000003E0) >> 5;
			regA = XW[n];
			regB = 0;
			regD = &pc;
			execcode = ADD;
			memCode = 0;
			wb = 1;
			sair = true;
			_float = false;
			break;
	}
}

//Executa a operação especificada por execcode, e realiza os acessos à memória
void exec(){
	switch(execcode){
		/*
		 * PONTO FLUTUANTE
		 */
		case fADD:
			fResult = fRegA+fRegB;
			break;
			
		case fSUB:
			fResult = fRegA-fRegB;
			break;
			
		case fDIV:
			fResult = fRegB/fRegA;
			break;
			
		case fLDR:
			result = regA + regB;
			break;
			
		case fSTR:
			result = regA + regB;
			break;
			
		case fNEG:
			fResult = 0 - fRegA;
			break;
			
		case fMOV:
			fResult = exec8to32(regA);
			break;
			
		case fMUL:
			fResult = fRegA * fRegB;
			break;
			
			/*
			 * INTEIRO
			 */
		case ADD:
			result = regA+regB;
			if (result<0)   neg = 1;
			else neg = 0;
			if (result==0) zero = 1;
			else zero = 0;
			if ((regA > 0 && regB > 0 && neg) || (regA < 0 && regB < 0 && !neg))
				overflow = 1;
			else
				overflow = 0;
			break;
			
		case SUB:
			result = regA-regB;
			if (result<0) neg = 1;
			else neg = 0;
			if (result==0) zero = 1;
			else zero = 0;
			if((regA < 0 && regB > 0 && !neg) || (regA > 0 && regB < 0 && neg)){ 
				overflow = 1;
			}
			else overflow = 0;
			break;
			
		case RIGHT:
			result = regA>>regB;
			break;
			
		case LEFT:
			result = regA<<regB;
			break;
			
		case MOV:
			result = regB;
			break;
			
		default:
			//cout << "NAO IMPLEMENTADO";
			exit(1);
			break;
		}
}



void mem() {    //PRONTO
	//tratar para 32 bits ou 64 bits
	switch (memCode){
		case 1: //Leitura na memória 32 bits
			result = (unsigned long)((signed long) ((signed int)memory[result>>2])); // extends
			arquivo << "rd %08lX " << (unsigned long) result << endl ;
			break;
		case 2: //Escrita na memória 32 bits
			memory[result>>2] = (unsigned int)*regD;
			arquivo << "wd %08lX " << (unsigned long) result << endl;
			break;
		case 3: //Leitura na memória 64 bits
			result = ((unsigned long *)memory)[result>>3];
			arquivo << "rd %08lX " << (unsigned long) result << endl;
			break;
		case 4: //Escrita na memória 64 bits
			((unsigned long *)memory)[result>>3] = *regD;
			arquivo << "wd %08lX " << (unsigned long) result << endl;
			break;
		case 5: //Leitura na memória 32 bits sem extensão de sinal
				result = (unsigned long)((unsigned int)memory[result>>2]); // extends sem sinal
				arquivo << "rd %08lX " << (unsigned long) result << endl ;
			break;
			
		case 6: //Leitura na memória 32 bits float
			cout << "RESULT: " << result << endl << flush;
			fResult = ((float *)memory)[result>>2]; 
			arquivo << "rd %08lX " << (unsigned long) result << endl;
			break;
		case 7: //Escrita na memória 32 bits float
			((float *)memory)[result>>2] = *fRegD;
			arquivo << "wd %08lX " << (unsigned long) result << endl;
			break;
	}
	memCode = 0;
}


//Coloca o resultado da operação no registrador destino, quando necessário
void writeBack(){                                                       //PRONTA
	if (wb) {
		if (_float) {
			*fRegD = fResult;
		} else {
			*regD = result;
		}
	}
}


int main(int argc, char *argv[]) {
	const char *filename = "fpops.o";//mudar para string
	loadBinary(filename);
	writeFileAsText(filename);
	while (!sair){
		instructionFetch();
		instructionDecode();
		exec();
		mem();
		writeBack();
		pc = pc + 4;
	}
	arquivo.close();
	return 0;
}



