#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <map>
#include <fstream>
#include <sstream>

#include <sys/stat.h>


#include <Rcpp.h>


#define BUFFSIZE 10000  // max length of the header input file
#define FIRSTLINE 0  // keep at 0
#define FIRSTCOLUMN 0  // 1=first column corresponds to sample name/number
using namespace std;
using namespace Rcpp;

/////////////////////////////////////////////////////////////////////////////////////
//  ------------------           FUNCTIONS                 ---------------------
/////////////////////////////////////////////////////////////////////////////////////


/**
*  Transform data to factors
*/
void transformToFactors(int numSamples, std::vector< std::vector <std::string> > data, double** dataNumeric, int i ){


	 // create a dictionary to store the factors of the strings
 	map<string,int> myMap;

	//clean the dictionary since it is used column by column
	myMap.clear();
	myMap["NA"] = -1;
	myMap[""] = -1;
	int factor = 0;

 	for(int j = 0; j < numSamples; j++){
		map<string,int>::iterator it = myMap.find(data[j][i]);
		if ( it != myMap.end() ){
			dataNumeric[j][i] = it->second;
		}
		else {
			myMap[data[j][i]] = factor;
			dataNumeric[j][i] = factor;
			factor++;
		}
	}
}

bool setArrayValuesInt1(int* array, int length, int value){
	for(int i = 0; i < length; i++){
			array[i] = value;
		}
		return true;
}

int removeRowsAllNA(int numSamples, int numNodes, std::vector< std::vector <std::string> >& data){

	int* indexNA = new int[numSamples];
	setArrayValuesInt1(indexNA, numSamples, -1);
	int pos = 0;
	for(int i = 0; i < numSamples; i++){
		bool isNA = true;
		for(int j = 0; j < numNodes && isNA; j++){
			if((data[i][j].compare("NA")  != 0) && (data[i][j].compare("") != 0)){
				isNA = false;
			}
		}
		if(!isNA){
			indexNA[pos] = i;
			pos++;
		}
	}

	// if there are rows of NA value
	if(pos != 0){		
		//correct variable numSamples
		// save the values
		pos = 0;
		for(int i = 0; i < numSamples; i++){
			if(indexNA[i] != -1){
				for(int j = 0; j < numNodes; j++){
					data[pos][j] = data[indexNA[i]][j];
				}
				pos++;
			}
		}
		numSamples = pos;
	}

	return numSamples;
}


/**
*  Read input and fill all structures
*/
double** reading_input(int& row_num, int col_num, std::vector<std::string> vectorData, vector< vector <string> >&  state)
{


	bool isNA=false;
	vector <string> vec;

	//convert input data
	for(int i = 0; i < vectorData.size(); i++){
		if(i >= col_num){
			if(i % col_num == 0){
				if(i != col_num){
					state.push_back(vec);
					vec.clear();
				}
			}
			vec.push_back(vectorData[i]);
		}
	}

	state.push_back(vec);


	for(int i = 0; i < row_num; i++){
		for(int j = 0; j < col_num; j++){
			if((state[i][j].compare("NA")  == 0) || (state[i][j].compare("") == 0)){
				isNA = true;
			}
		}
	}

	
	if(isNA){
		//// Remove the lines that are all 'NA'
		row_num = removeRowsAllNA(row_num, col_num, state);
	}

	double** dataNumeric = new double*[row_num];
	for(int pos = 0; pos < row_num; pos++){
		dataNumeric[pos] = new double[col_num];
	}

	for(int i = 0; i < col_num; i++){
		transformToFactors(row_num, state, dataNumeric,  i);		
	}

	return dataNumeric;
}


/*
*	Evaluate the effective number of samples in the dataset
*/

int crosscorrelation(const int row_num, const int col_num, double **configuration, vector<double>& correlationV, int c_max){

  double sumk, sumi, norm;
  //int c_max = 5000;  // c_max < sample_num !!! t3
  //int c_max = 100;  // c_max < sample_num !!! test_file!
  
  double* Corr = new double[c_max];
  double* meanconf= new double[col_num];


  int i, k, c,neff;
  int start=0,shift=0;

  for(k=FIRSTCOLUMN; k<col_num; k++) {
    meanconf[k]=0.0;
    for(i=shift+FIRSTLINE; i<row_num; i++) {
      meanconf[k]=meanconf[k]+configuration[i][k];
	}
    meanconf[k]=meanconf[k]*1.0/(row_num-shift-FIRSTLINE);
  }

  for (c=start; c<c_max; c++) {  // c=0 : a configuration is equal to itself
    sumi=0;
    for(i=shift+FIRSTLINE; i<row_num-c; i++) {
      sumk=0;  
      for(k=FIRSTCOLUMN; k<col_num; k++) {
	sumk=sumk+(configuration[i][k]-meanconf[k])*(configuration[i+c][k]-meanconf[k]);
      }
      sumk=sumk/(col_num-FIRSTCOLUMN);
      //sumi=sumi+sumk*sumk;  // squared correlations
      sumi=sumi+sumk;  

    } 
    sumi=sumi/(row_num-c-shift-FIRSTLINE);
    if(c==start) norm=sumi;
    Corr[c]=sumi/norm;  

    correlationV.push_back(Corr[c]);
  }

  neff=floor(0.5+row_num*(1-Corr[1])/(1+Corr[1]));

  delete(Corr);
  delete(meanconf);

  return neff;

}


/*
*	Evaluate the effective number of samples in the dataset/ This function deals with R
*/

extern "C" SEXP evaluateEffn(SEXP inputDataR, SEXP variable_numR, SEXP sample_numR)
{
	vector<double> correlationV;
	std::vector<std::string>  vectorData;
	std::vector< std::vector <std::string> > state;

	vectorData = Rcpp::as< vector <string> > (inputDataR);

	int variable_num = Rcpp::as<int> (variable_numR);
	int sample_num = Rcpp::as<int> (sample_numR);

	int Neff,c_max=30;

	int i, j;

	double** dataNumeric = reading_input(sample_num, variable_num, vectorData, state);

	//  Compute crosscorrelation C(c) and identify R:
	Neff=crosscorrelation(sample_num, variable_num, dataNumeric, correlationV, c_max);

	if(Neff > sample_num)
		Neff = sample_num;
  

  // structure the output
    List result = List::create(
	 	_["correlation"] = correlationV,
        _["neff"] = Neff
       // _["scores"] = outScore
    ) ;
    return result;
}
