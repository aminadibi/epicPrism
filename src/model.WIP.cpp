/*
Layout:
1. Basic
2. Settings
3. Radom
4. Input
5. Output
6. Agent
7. Event  
8. Model 
*/
 




#define _VERSION_ "20160924"
#define _VERSION_DESC_ "NOTEs:                                 \
  1. comorbidity events are disabled (in the main loop)        \
  2.                                                           \
  "
//TODO;
#define OUTPUT_EX_BIOMETRICS 1 //height, weight etc;
#define OUTPUT_EX_SMOKING 2
#define OUTPUT_EX_COMORBIDITY 4
#define OUTPUT_EX_LUNG_FUNCTION 8
#define OUTPUT_EX_COPD 16
#define OUTPUT_EX_EXACERBATION 32
#define OUTPUT_EX_MORTALITY 64
#define OUTPUT_EX_MEDICATION 128
#define OUTPUT_EX_POPULATION 256

#define OUTPUT_EX 65535




#define PROJECT_ROC16       //Hypothetical prognostic biomarker of exacerbation


#define MAX_AGE 111



#include <Rcpp.h>
using namespace Rcpp; 






enum errors
{
  ERR_INCORRECT_SETTING_VARIABLE=-1,
  ERR_INCORRECT_VECTOR_SIZE=-2,
  ERR_INCORRECT_INPUT_VAR=-3,
  ERR_EVENT_STACK_FULL=-4,
  ERR_MEMORY_ALLOCATION_FAILED=-5
} errors;
/*** R
  errors<-c(
  ERR_INCORRECT_SETTING_VARIABLE=-1,
  ERR_INCORRECT_VECTOR_SIZE=-2,
  ERR_INCORRECT_INPUT_VAR=-3,
  ERR_EVENT_STACK_FULL=-4,
  ERR_MEMORY_ALLOCATION_FAILED=-5
  )
*/



enum record_mode
{
  record_mode_none=0,
  record_mode_agent=1,
  record_mode_event=2,
  record_mode_some_event=3
};
/*** R
  record_mode<-c(
  record_mode_none=0,
  record_mode_agent=1,
  record_mode_event=2,
  record_mode_some_event=3
  )
*/



enum agent_creation_mode
{
  agent_creation_mode_one=0,
  agent_creation_mode_all=1,
  agent_creation_mode_pre=2
};
/*** R
  agent_creation_mode<-c(
  agent_creation_mode_one=0,
  agent_creation_mode_all=1,
  agent_creation_mode_pre=2
  )
*/




enum medication_classes
{
  MED_CLASS_SABA=1,
  MED_CLASS_LABA=2,
  MED_CLASS_LAMA=4,
  MED_CLASS_ICS=8,
  MED_CLASS_MACRO=16,
    N_MED_CLASS=5 //need to update this if new medication is added
};
/*** R
  medication_classes<-c(
  MED_CLASS_SABA=1,
  MED_CLASS_LABA=2,
  MED_CLASS_LAMA=4,
  MED_CLASS_ICS=8,
  MED_CLASS_MACRO=16
  )
*/























/////////////////////////////////////////////////////////////////////BASICS//////////////////////////////////////////////
#define max(a,b)            \
({ __typeof__ (a) _a = (a); \
  __typeof__ (b) _b = (b);  \
  _a > _b ? _a : _b; })     \
    
  
#define min(a,b)              \
  ({ __typeof__ (a) _a = (a); \
  __typeof__ (b) _b = (b);    \
  _a > _b ? _b : _a; })       \
    


double calendar_time;
int last_id;




NumericMatrix array_to_Rmatrix(std::vector<double> x, int nCol)
{
  int nRow=x.size()/nCol;
  NumericMatrix y(nRow,nCol);
  //important: CPP is column major order but R is row major; we address the R matrix cell by cell but handle the vector in CPP way;
  for(int i=0;i<nRow;i++)
    for(int j=0;j<nCol;j++)
      y(i,j)=x[i*nCol+j];
  return (y);
}


NumericMatrix array_to_Rmatrix(std::vector<int> x, int nCol)
{
  int nRow=x.size()/nCol;
  NumericMatrix y(nRow,nCol);
  //important: CPP is column major order but R is row major; we address the R matrix cell by cell but handle the vector in CPP way;
  for(int i=0;i<nRow;i++)
    for(int j=0;j<nCol;j++)
      y(i,j)=x[i*nCol+j];
  return (y);
}




  





#define AS_VECTOR_DOUBLE(src) std::vector<double>(&src[0],&src[0]+sizeof(src)/sizeof(double))
#define AS_VECTOR_DOUBLE_SIZE(src,size) std::vector<double>(&src[0],&src[0]+size)

#define AS_MATRIX_DOUBLE(src)  array_to_Rmatrix(std::vector<double>(&src[0][0],&src[0][0]+sizeof(src)/sizeof(double)),sizeof(src[0])/sizeof(double))
#define AS_MATRIX_DOUBLE_SIZE(src,size)  array_to_Rmatrix(std::vector<double>(&src[0][0],&src[0][0]+size*sizeof(src[0])/sizeof(double)),sizeof(src[0])/sizeof(double))

#define AS_MATRIX_INT(src)  array_to_Rmatrix(std::vector<int>(&src[0][0],&src[0][0]+sizeof(src)/sizeof(int)),sizeof(src[0])/sizeof(int))
#define AS_MATRIX_INT_SIZE(src,size)  array_to_Rmatrix(std::vector<int>(&src[0][0],&src[0][0]+size*sizeof(src[0])/sizeof(int)),sizeof(src[0])/sizeof(int))

#define AS_VECTOR_INT(src) std::vector<int>(&src[0],&src[0]+sizeof(src)/sizeof(int))
#define AS_VECTOR_INT_SIZE(src,size) std::vector<int>(&src[0],&src[0]+size)

#define READ_R_VECTOR(src,dest) {if(src.size()==sizeof(dest)/sizeof(dest[0])) {std::copy(src.begin(),src.end(),&dest[0]); return(0);} else return(ERR_INCORRECT_VECTOR_SIZE);}

#define READ_R_MATRIX(src,dest) {if(src.size()==sizeof(dest)/sizeof(dest[0][0])) {std::copy(src.begin(),src.end(),&dest[0][0]); return(0);} else return(ERR_INCORRECT_VECTOR_SIZE);}



//////////////////////////////////////////////////////////////////////SETTINGS//////////////////////////////////////////////////



struct settings
{
  int record_mode;    //0: nothing recorded, 1:agents recorded, 2: events recorded, 3:selected events recorded;
  
  int events_to_record[100];  //valid only if record_mode=3
  int n_events_to_record;
  
  int agent_creation_mode; //0: one agent at a time; 1: agents are created and saved in agent_stack. 2: saved agents in agent_stack are used (require create_agents before running the model)
  
  int update_continuous_outcomes_mode;    //0:update only on fixed and end events; 1: update before any event;
  
  int n_base_agents;
  
  int runif_buffer_size;
  int rnorm_buffer_size;
  int rexp_buffer_size;
  
  int agent_stack_size;
  int event_stack_size;
} settings;




// [[Rcpp::export]]
int Cset_settings_var(std::string name,NumericVector value)
{
  if(name=="record_mode") {settings.record_mode=value[0]; return(0);}
  if(name=="events_to_record") 
  {
    settings.n_events_to_record=0;
    for(int i=0;i<value.size();i++)
    {
      settings.events_to_record[i]=value[i];
      settings.n_events_to_record++;
    }
    return(0);
  }
  if(name=="agent_creation_mode") {settings.agent_creation_mode=value[0]; return(0);}
  if(name=="update_continuous_outcomes_mode") {settings.update_continuous_outcomes_mode=value[0]; return(0);}
  if(name=="n_base_agents") {settings.n_base_agents=value[0]; return(0);}
  if(name=="runif_buffer_size") {settings.runif_buffer_size=value[0]; return(0);}
  if(name=="rnorm_buffer_size") {settings.rnorm_buffer_size=value[0]; return(0);}
  if(name=="rexp_buffer_size") {settings.rexp_buffer_size=value[0]; return(0);}
  if(name=="agent_stack_size") {settings.agent_stack_size=value[0]; return(0);}
  if(name=="event_stack_size") {settings.event_stack_size=value[0]; return(0);}
  return(ERR_INCORRECT_SETTING_VARIABLE);
}



// [[Rcpp::export]]
List Cget_settings()
{
 return Rcpp::List::create(
      Rcpp::Named("record_mode")=settings.record_mode,
      Rcpp::Named("events_to_record")=AS_VECTOR_DOUBLE_SIZE(settings.events_to_record,settings.n_events_to_record),
      Rcpp::Named("agent_creation_mode")=settings.agent_creation_mode,
      Rcpp::Named("update_continuous_outcomes_mode")=settings.update_continuous_outcomes_mode,
      Rcpp::Named("n_base_agents")=settings.n_base_agents,
      Rcpp::Named("runif_buffer_size")=settings.runif_buffer_size,
      Rcpp::Named("rnorm_buffer_size")=settings.rnorm_buffer_size,
      Rcpp::Named("rexp_buffer_size")=settings.rexp_buffer_size,
      Rcpp::Named("agent_stack_size")=settings.agent_stack_size,
      Rcpp::Named("event_stack_size")=settings.event_stack_size
    );
}



// [[Rcpp::export]]
List Cget_version()
{
  return Rcpp::List::create(
    Rcpp::Named("version")=_VERSION_,
    Rcpp::Named("version_desc")=_VERSION_DESC_
  );
}
  
  
  
  


struct runtime_stats
{
  int agent_size;
  int n_runif_fills;
  int n_rnorm_fills;
  int n_rexp_fills;
} runtime_stats;


void reset_runtime_stats()
{
  char *x=reinterpret_cast <char *>(&runtime_stats);
  for(unsigned i=0;i<sizeof(runtime_stats);i++)
    x[i]=0;
}


// [[Rcpp::export]]
List Cget_runtime_stats()
{
 return Rcpp::List::create(
      Rcpp::Named("agent_size")=runtime_stats.agent_size,
      Rcpp::Named("n_runif_fills")=runtime_stats.n_runif_fills,
      Rcpp::Named("n_rnorm_fills")=runtime_stats.n_rnorm_fills,
      Rcpp::Named("n_rexp_fills")=runtime_stats.n_rexp_fills
    );
}





////////////////////////////////////////////////////////////////////RANDOM/////////////////////////////////////////////////
//these stuff are internal so no expoert/import;
double *runif_buffer;
long runif_buffer_pointer;

double *rnorm_buffer;
long rnorm_buffer_pointer;

double *rexp_buffer;
long rexp_buffer_pointer;
  
  


double* R_runif(int n)
{
  NumericVector temp(runif(n));
  return(&temp(0));
} 
double* R_runif(int n, double * address)
{
  NumericVector temp(runif(n));
  std::copy(temp.begin(),temp.end(),address);
  return(address);
}
int runif_fill()
{
  R_runif(settings.runif_buffer_size,runif_buffer);
  runif_buffer_pointer=0;
  ++runtime_stats.n_runif_fills;
  return(0);
}
double rand_unif()
{
  if(runif_buffer_pointer==settings.runif_buffer_size) {runif_fill();}
  double temp=runif_buffer[runif_buffer_pointer];
  runif_buffer_pointer++;
  return(temp);
}




double* R_rnorm(int n)
{
  NumericVector temp(rnorm(n));
  return(&temp(0));
}
double* R_rnorm(int n, double * address)
{
  NumericVector temp(rnorm(n));
  std::copy(temp.begin(),temp.end(),address);
  return(address);
}
int rnorm_fill()
{
  R_rnorm(settings.rnorm_buffer_size,rnorm_buffer);
  rnorm_buffer_pointer=0;
  ++runtime_stats.n_rnorm_fills;
  return(0);
}
double rand_norm()
{
  if(rnorm_buffer_pointer==settings.rnorm_buffer_size) {rnorm_fill();}
  double temp=rnorm_buffer[rnorm_buffer_pointer];
  rnorm_buffer_pointer++;
  return(temp);
}

//bivariate normal, method 1: standard normal with rho;
void rbvnorm(double rho, double x[2])
{
  x[0]=rand_norm();
  double mu=rho*x[0];
  double v=(1-rho*rho);
  
  x[1]=rand_norm()*sqrt(v)+mu;
}





double* R_rexp(int n)
{
  NumericVector temp(rexp(n,1));
  return(&temp(0));
}
double* R_rexp(int n, double * address)
{
  NumericVector temp(rexp(n,1));
  std::copy(temp.begin(),temp.end(),address);
  return(address);
}
int rexp_fill()
{
  R_rexp(settings.rexp_buffer_size,rexp_buffer);
  rexp_buffer_pointer=0;
  ++runtime_stats.n_rexp_fills;
  return(0);
}
double rand_exp()
{
  if(rexp_buffer_pointer==settings.rexp_buffer_size) {rexp_fill();}
  double temp=rexp_buffer[rexp_buffer_pointer];
  rexp_buffer_pointer++;
  return(temp);
}


// [[Rcpp::export]]
NumericVector Xrexp(int n, double rate)
{
  double *temp=R_rexp(n);
  return(temp[0]/rate);
}













////////////////////////////////////////////////////////////////////INPUT/////////////////////////////////////////////
struct input
{
  struct
  {
    int time_horizon;
    double y0;  //calendar year
    double age0;  //age to start
    double discount_cost;
    double discount_qaly;
  } global_parameters;
  
  struct 
  {
    double p_female;
    
    double height_0_betas[5]; //intercept, sex, age, age2, sex*age;
    double height_0_sd;
    double weight_0_betas[7]; //intercept, sex, age, age2, sex*age, height, year;
    double weight_0_sd;  //currently, a sample is made at baseline and every one is moved in parallel trajectories
    double height_weight_rho;
    
    double p_prevalence_age[111]; //age distirbution of prevalent (time_at_creation=0) agents
    double p_incidence_age[111]; //age distirbution of incident (time_at_creation>0) agents
    double l_inc_betas[3]; //intercept, clandar year and its square
    double p_bgd_by_sex[111][2];
    double ln_h_bgd_betas[9]; //intercept, calendar year, its square, age, b_mi, n_mi, b_stroke, n_stroke, hf_status
  } agent;
  
  
  struct
  {
    double logit_p_current_smoker_0_betas[7]; //intercept, sex, age, age2, age*sex sex*age^2 year;
    double logit_p_ever_smoker_con_not_current_0_betas[7]; //intercept, sex, age, age2,age*sex sex*age^2 year;
    double pack_years_0_betas[5]; //intercept, smoker, sex, age, year
    double pack_years_0_sd;
    double ln_h_inc_betas[5]; //intercept, sex, age, age*2 calendar time, 
    double ln_h_ces_betas[5]; //intercept, sex, age, age*2 calendar time, 
  } smoking;
  
  
  struct
  {
    double ln_h_COPD_betas_by_sex[7][2]; //INCIDENCE: intercept, age, age2, pack_years, current_smoking, year, asthma;
    double logit_p_COPD_betas_by_sex[7][2]; //PREVALENCE: intercept, age, age2, pack_years, current_smoking, year, asthma;
  } COPD;
  
  
  struct
  {
    double fev1_0_prev_betas_by_sex[6][2];   //Initiating FEV1 in prevalent COPD cases (FEV1/FVC<0.7): intercept, age, height, pack_years
    double fev1_0_prev_sd_by_sex[2];
    
    double fev1_0_inc_betas_by_sex[6][2];   //Initiating FEV1 in incident COPD cases (FEV1/FVC=0.7): intercept, age, height, pack_years
    double fev1_0_inc_sd_by_sex[2]; 
    
    double pred_fev1_betas_by_sex[4][2]; //Predicted FEV1, intercept, age, height, RESERVED
    
    double dfev1_betas[6];  //intercept, sex, age, FEV1_0, smoking
    double dfev1_re_sds[2];
    double dfev1_re_rho;
  } lung_function;
  

  struct
  {
    double ln_rate_betas[5];     //intercept sex age fev1 smoking status
    double logit_severity_betas[6];     //intercept1, intercept2, sex age fev1 smoking_status
    double ln_rate_intercept_sd;
    double logit_severity_intercept_sd;       //sd of the intercept (random-effects)
    double rate_severity_intercept_rho;
    //double p_moderate_severe[2]; //probability of moderate or severe, compared with mild, exacerbation
    double exac_end_rate[3]; //rate of exiting exacerbation per type;
    double p_death[3]; //rate of mortality per type;
  } exacerbation;
  
  
  
  struct
  {
    double bg_cost_by_stage[5];
    double exac_dcost[3];
    double doctor_visit_by_type[2];
    
    double mi_dcost;
    double mi_post_cost;
    double stroke_dcost;
    double stroke_post_dcost;
  } cost;
  
  struct
  {
    double bg_util_by_stage[5];
    double exac_dutil[3];
    
    double mi_dutil;
    double mi_post_dutil;
    double stroke_dutil;
    double stroke_post_dutil;    
  } utility;
  
   
  struct 
  {
    double rate_doctor_visit;
    double p_specialist;
  } outpatient;
  
  struct 
  {
    double ln_h_start_betas_by_class[N_MED_CLASS][3+N_MED_CLASS];
    double ln_h_stop_betas_by_class[N_MED_CLASS][3+N_MED_CLASS];
    double ln_rr_exac_by_class[N_MED_CLASS];
  } medication;
  
  struct
  {
    double logit_p_mi_betas_by_sex[8][2];   //intercept, age, age2, pack_years, smoking, calendar time, bmi, gold stage, ???
    double ln_h_mi_betas_by_sex[10][2]; //intercept, age, age2, pack_years, smoking_status, calendar time, bmi, gold stage, n_mi
    double p_mi_death;
    
    double logit_p_stroke_betas_by_sex[10][2]; 
    double ln_h_stroke_betas_by_sex[12][2];
    double p_stroke_death;
    
    double logit_p_hf_betas_by_sex[12][2];
    double ln_h_hf_betas_by_sex[12][2];
    double p_hf_x;
  } comorbidity;
  
  struct
  {
    double ROC16_biomarker_noise_sd;
    double ROC16_biomarker_threshold;
    double ROC16_treatment_RR;
    double ROC16_biomarker_cost;
    double ROC16_treatment_cost;
  } project_specific;
  
  
} input;






// [[Rcpp::export]]
List Cget_inputs()
{
 List out=Rcpp::List::create(
      Rcpp::Named("global_parameters")=Rcpp::List::create(
      Rcpp::Named("age0")=input.global_parameters.age0,
      Rcpp::Named("time_horizon")=input.global_parameters.time_horizon,
      Rcpp::Named("y0")=input.global_parameters.y0,
      Rcpp::Named("discount_cost")=input.global_parameters.discount_cost,
      Rcpp::Named("discount_qaly")=input.global_parameters.discount_qaly
      ),
    Rcpp::Named("agent")=Rcpp::List::create(
      Rcpp::Named("p_female")=input.agent.p_female,
      Rcpp::Named("height_0_betas")=AS_VECTOR_DOUBLE(input.agent.height_0_betas),
      Rcpp::Named("height_0_sd")=input.agent.height_0_sd,
      Rcpp::Named("weight_0_betas")=AS_VECTOR_DOUBLE(input.agent.weight_0_betas),
      Rcpp::Named("weight_0_sd")=input.agent.weight_0_sd,
      Rcpp::Named("height_weight_rho")=input.agent.height_weight_rho,
      
      Rcpp::Named("p_prevalence_age")=AS_VECTOR_DOUBLE(input.agent.p_prevalence_age),
      Rcpp::Named("p_incidence_age")=AS_VECTOR_DOUBLE(input.agent.p_incidence_age),
      Rcpp::Named("p_bgd_by_sex")=AS_MATRIX_DOUBLE(input.agent.p_bgd_by_sex),
      Rcpp::Named("l_inc_betas")=AS_VECTOR_DOUBLE(input.agent.l_inc_betas),
      Rcpp::Named("ln_h_bgd_betas")=AS_VECTOR_DOUBLE(input.agent.ln_h_bgd_betas)
      ),
    Rcpp::Named("smoking")=Rcpp::List::create(
      //Rcpp::Named("inc_by_age_sex")=AS_MATRIX_DOUBLE(input.smoking.inc_by_age_sex),
      //Rcpp::Named("ces_by_age_sex")=AS_MATRIX_DOUBLE(input.smoking.ces_by_age_sex),
      //Rcpp::Named("rel_by_age_sex")=AS_MATRIX_DOUBLE(input.smoking.rel_by_age_sex),
         
      Rcpp::Named("logit_p_current_smoker_0_betas")=AS_VECTOR_DOUBLE(input.smoking.logit_p_current_smoker_0_betas),
      Rcpp::Named("logit_p_ever_smoker_con_not_current_0_betas")=AS_VECTOR_DOUBLE(input.smoking.logit_p_ever_smoker_con_not_current_0_betas),
      Rcpp::Named("pack_years_0_betas")=AS_VECTOR_DOUBLE(input.smoking.pack_years_0_betas),
      Rcpp::Named("pack_years_0_sd")=input.smoking.pack_years_0_sd,
      Rcpp::Named("ln_h_inc_betas")=AS_VECTOR_DOUBLE(input.smoking.ln_h_inc_betas),
      Rcpp::Named("ln_h_ces_betas")=AS_VECTOR_DOUBLE(input.smoking.ln_h_ces_betas)
      ),
    Rcpp::Named("COPD")=Rcpp::List::create(
      Rcpp::Named("ln_h_COPD_betas_by_sex")=AS_MATRIX_DOUBLE(input.COPD.ln_h_COPD_betas_by_sex),
      Rcpp::Named("logit_p_COPD_betas_by_sex")=AS_MATRIX_DOUBLE(input.COPD.logit_p_COPD_betas_by_sex)
      ),
    Rcpp::Named("lung_function")=Rcpp::List::create(
      Rcpp::Named("pred_fev1_betas_by_sex")=AS_MATRIX_DOUBLE(input.lung_function.pred_fev1_betas_by_sex),
      Rcpp::Named("fev1_0_prev_betas_by_sex")=AS_MATRIX_DOUBLE(input.lung_function.fev1_0_prev_betas_by_sex),
      Rcpp::Named("fev1_0_prev_sd_by_sex")=AS_VECTOR_DOUBLE(input.lung_function.fev1_0_prev_sd_by_sex),
      Rcpp::Named("fev1_0_inc_betas_by_sex")=AS_MATRIX_DOUBLE(input.lung_function.fev1_0_prev_betas_by_sex),
      Rcpp::Named("fev1_0_inc_sd_by_sex")=AS_VECTOR_DOUBLE(input.lung_function.fev1_0_prev_sd_by_sex),
      Rcpp::Named("dfev1_betas")=AS_VECTOR_DOUBLE(input.lung_function.dfev1_betas),
      Rcpp::Named("dfev1_re_sds")=AS_VECTOR_DOUBLE(input.lung_function.dfev1_re_sds),
      Rcpp::Named("dfev1_re_rho")=input.lung_function.dfev1_re_rho
      ),
    Rcpp::Named("exacerbation")=Rcpp::List::create(
      Rcpp::Named("ln_rate_betas")=AS_VECTOR_DOUBLE(input.exacerbation.ln_rate_betas),
      Rcpp::Named("logit_severity_betas")=AS_VECTOR_DOUBLE(input.exacerbation.logit_severity_betas),
      Rcpp::Named("ln_rate_intercept_sd")=input.exacerbation.ln_rate_intercept_sd,
      Rcpp::Named("logit_severity_intercept_sd")=input.exacerbation.logit_severity_intercept_sd,
      Rcpp::Named("rate_severity_intercept_rho")=input.exacerbation.rate_severity_intercept_rho,
      //Rcpp::Named("p_moderate_severe")=AS_VECTOR_DOUBLE(input.exacerbation.p_moderate_severe),
      Rcpp::Named("exac_end_rate")=AS_VECTOR_DOUBLE(input.exacerbation.exac_end_rate),
      Rcpp::Named("p_death")=AS_VECTOR_DOUBLE(input.exacerbation.p_death)
      ),
    Rcpp::Named("outpatient")=Rcpp::List::create(
      Rcpp::Named("rate_doctor_visit")=input.outpatient.rate_doctor_visit,
      Rcpp::Named("p_specialist")=input.outpatient.p_specialist
      ),
    Rcpp::Named("comorbidity")=Rcpp::List::create(
      Rcpp::Named("logit_p_mi_betas_by_sex")=AS_MATRIX_DOUBLE(input.comorbidity.logit_p_mi_betas_by_sex),
      Rcpp::Named("ln_h_mi_betas_by_sex")=AS_MATRIX_DOUBLE(input.comorbidity.ln_h_mi_betas_by_sex),
      Rcpp::Named("p_mi_death")=input.comorbidity.p_mi_death,
      Rcpp::Named("logit_p_stroke_betas_by_sex")=AS_MATRIX_DOUBLE(input.comorbidity.logit_p_stroke_betas_by_sex),
      Rcpp::Named("ln_h_stroke_betas_by_sex")=AS_MATRIX_DOUBLE(input.comorbidity.ln_h_stroke_betas_by_sex),
      Rcpp::Named("p_stroke_death")=input.comorbidity.p_stroke_death,
      Rcpp::Named("logit_p_hf_betas_by_sex")=AS_MATRIX_DOUBLE(input.comorbidity.logit_p_hf_betas_by_sex),
      Rcpp::Named("ln_h_hf_betas_by_sex")=AS_MATRIX_DOUBLE(input.comorbidity.ln_h_hf_betas_by_sex)
      ),
      
    Rcpp::Named("cost")=Rcpp::List::create(
      Rcpp::Named("bg_cost_by_stage")=AS_VECTOR_DOUBLE(input.cost.bg_cost_by_stage),
      Rcpp::Named("exac_dcost")=AS_VECTOR_DOUBLE(input.cost.exac_dcost),
      Rcpp::Named("doctor_visit_by_type")=AS_VECTOR_DOUBLE(input.cost.doctor_visit_by_type)
      ),
    Rcpp::Named("utility")=Rcpp::List::create(
      Rcpp::Named("bg_util_by_stage")=AS_VECTOR_DOUBLE(input.utility.bg_util_by_stage),
      Rcpp::Named("exac_dutil")=AS_VECTOR_DOUBLE(input.utility.exac_dutil)
      )
      ,
    Rcpp::Named("medication")=Rcpp::List::create(
      Rcpp::Named("ln_h_start_betas_by_class")=AS_MATRIX_DOUBLE(input.medication.ln_h_start_betas_by_class),
      Rcpp::Named("ln_h_stop_betas_by_class")=AS_MATRIX_DOUBLE(input.medication.ln_h_stop_betas_by_class),
      Rcpp::Named("ln_rr_exac_by_class")=AS_VECTOR_DOUBLE(input.medication.ln_rr_exac_by_class)
      )
      ,
    Rcpp::Named("project_specific")=Rcpp::List::create(
#ifdef PROJECT_ROC16
      Rcpp::Named("ROC16_biomarker_noise_sd")=input.project_specific.ROC16_biomarker_noise_sd,
      Rcpp::Named("ROC16_biomarker_threshold")=input.project_specific.ROC16_biomarker_threshold,
      Rcpp::Named("ROC16_treatment_RR")=input.project_specific.ROC16_treatment_RR,
      Rcpp::Named("ROC16_biomarker_cost")=input.project_specific.ROC16_biomarker_cost,
      Rcpp::Named("ROC16_treatment_cost")=input.project_specific.ROC16_treatment_cost
#endif
      )
    );
    
    return(out);
}




// [[Rcpp::export]]
int Cset_input_var(std::string name,NumericVector value)
{
  if(name=="global_parameters$age0") {input.global_parameters.age0=value[0]; return(0);} 
  if(name=="global_parameters$time_horizon")  {input.global_parameters.time_horizon=value[0]; return(0);}  
  if(name=="global_parameters$discount_cost") {input.global_parameters.discount_cost=value[0]; return(0);}  
  if(name=="global_parameters$discount_qaly") {input.global_parameters.discount_qaly=value[0]; return(0);}  
  
  if(name=="agent$p_female") {input.agent.p_female=value[0]; return(0);}
  if(name=="agent$p_prevalence_age") READ_R_VECTOR(value,input.agent.p_prevalence_age);
  if(name=="agent$height_0_betas") READ_R_VECTOR(value,input.agent.height_0_betas);
  if(name=="agent$height_0_sd") {input.agent.height_0_sd=value[0]; return(0);}
  if(name=="agent$weight_0_betas") READ_R_VECTOR(value,input.agent.weight_0_betas);
  if(name=="agent$weight_0_sd") {input.agent.weight_0_sd=value[0]; return(0);}
  if(name=="agent$height_weight_rho") {input.agent.height_weight_rho=value[0]; return(0);}
    
  if(name=="agent$p_incidence_age") READ_R_VECTOR(value,input.agent.p_incidence_age);
  if(name=="agent$p_bgd_by_sex") READ_R_MATRIX(value,input.agent.p_bgd_by_sex);
  if(name=="agent$l_inc_betas") READ_R_VECTOR(value,input.agent.l_inc_betas);
  if(name=="agent$ln_h_bgd_betas") READ_R_VECTOR(value,input.agent.ln_h_bgd_betas);
  
  //if(name=="smoking$inc_by_age_sex") READ_R_MATRIX(value,input.smoking.inc_by_age_sex);
  //if(name=="smoking$ces_by_age_sex") READ_R_MATRIX(value,input.smoking.ces_by_age_sex);
  //if(name=="smoking$rel_by_age_sex") READ_R_MATRIX(value,input.smoking.rel_by_age_sex);
  
  if(name=="smoking$logit_p_current_smoker_0_betas") READ_R_VECTOR(value,input.smoking.logit_p_current_smoker_0_betas);
  if(name=="smoking$logit_p_ever_smoker_con_not_current_0_betas") READ_R_VECTOR(value,input.smoking.logit_p_ever_smoker_con_not_current_0_betas);
  if(name=="smoking$pack_years_0_betas") READ_R_VECTOR(value,input.smoking.pack_years_0_betas);
  if(name=="smoking$pack_years_0_sd") {input.smoking.pack_years_0_sd=value[0]; return(0);}
  if(name=="smoking$ln_h_inc_betas") READ_R_VECTOR(value,input.smoking.ln_h_inc_betas);
  if(name=="smoking$ln_h_ces_betas") READ_R_VECTOR(value,input.smoking.ln_h_ces_betas);
 
  if(name=="COPD$ln_h_COPD_betas_by_sex") READ_R_MATRIX(value,input.COPD.ln_h_COPD_betas_by_sex);
  if(name=="COPD$logit_p_COPD_betas_by_sex") READ_R_MATRIX(value,input.COPD.logit_p_COPD_betas_by_sex);
    
  if(name=="lung_function$pred_fev1_betas_by_sex") READ_R_MATRIX(value,input.lung_function.pred_fev1_betas_by_sex);
  if(name=="lung_function$fev1_0_prev_betas_by_sex") READ_R_MATRIX(value,input.lung_function.fev1_0_prev_betas_by_sex);
  if(name=="lung_function$fev1_0_prev_sd_by_sex") READ_R_VECTOR(value,input.lung_function.fev1_0_prev_sd_by_sex);
  if(name=="lung_function$fev1_0_inc_betas_by_sex") READ_R_MATRIX(value,input.lung_function.fev1_0_inc_betas_by_sex);
  if(name=="lung_function$fev1_0_inc_sd_by_sex") READ_R_VECTOR(value,input.lung_function.fev1_0_inc_sd_by_sex);
  if(name=="lung_function$pred_fev1_betas_by_sex") READ_R_MATRIX(value,input.lung_function.pred_fev1_betas_by_sex);
  if(name=="lung_function$dfev1_betas") READ_R_VECTOR(value,input.lung_function.dfev1_betas);
  if(name=="lung_function$dfev1_re_sds") READ_R_VECTOR(value,input.lung_function.dfev1_re_sds);
  if(name=="lung_function$dfev1_re_rho") {input.lung_function.dfev1_re_rho=value[0]; return(0);}
  
  if(name=="exacerbation$ln_rate_betas") READ_R_VECTOR(value,input.exacerbation.ln_rate_betas);
  if(name=="exacerbation$logit_severity_betas") READ_R_VECTOR(value,input.exacerbation.logit_severity_betas);
  if(name=="exacerbation$ln_rate_intercept_sd") {input.exacerbation.ln_rate_intercept_sd=value[0]; return(0);}
  if(name=="exacerbation$logit_severity_intercept_sd") {input.exacerbation.logit_severity_intercept_sd=value[0]; return(0);}
  if(name=="exacerbation$rate_severity_intercept_rho") {input.exacerbation.rate_severity_intercept_rho=value[0]; return(0);}
  //if(name=="exacerbation$p_moderate_severe") READ_R_VECTOR(value,input.exacerbation.p_moderate_severe);
  if(name=="exacerbation$exac_end_rate") READ_R_VECTOR(value,input.exacerbation.exac_end_rate);
  if(name=="exacerbation$p_death") READ_R_VECTOR(value,input.exacerbation.p_death);
  
  if(name=="outpatient$rate_doctor_visit") {input.outpatient.rate_doctor_visit=value[0]; return(0);}
  if(name=="outpatient$p_specialist") {input.outpatient.p_specialist=value[0]; return(0);}
  
  if(name=="cost$bg_cost_by_stage") READ_R_VECTOR(value,input.cost.bg_cost_by_stage);
  
  if(name=="medication$ln_h_start_betas_by_class") READ_R_MATRIX(value,input.medication.ln_h_start_betas_by_class);
  if(name=="medication$ln_h_stop_betas_by_class") READ_R_MATRIX(value,input.medication.ln_h_stop_betas_by_class);
  if(name=="medication$ln_rr_exac_by_class") READ_R_VECTOR(value,input.medication.ln_rr_exac_by_class);


  if(name=="cost$exac_dcost") READ_R_VECTOR(value,input.cost.exac_dcost);
  if(name=="cost$doctor_visit_by_type") READ_R_VECTOR(value,input.cost.doctor_visit_by_type);
  
  if(name=="utility$bg_util_by_stage") READ_R_VECTOR(value,input.utility.bg_util_by_stage);
  if(name=="utility$exac_dutil") READ_R_VECTOR(value,input.utility.exac_dutil);
   
  if(name=="comorbidity$logit_p_mi_betas_by_sex") READ_R_MATRIX(value,input.comorbidity.logit_p_mi_betas_by_sex);
  if(name=="comorbidity$ln_h_mi_betas_by_sex") READ_R_MATRIX(value,input.comorbidity.ln_h_mi_betas_by_sex);
  if(name=="comorbidity$p_mi_death") {input.comorbidity.p_mi_death=value[0]; return(0);}
  if(name=="comorbidity$logit_p_stroke_betas_by_sex") READ_R_MATRIX(value,input.comorbidity.logit_p_stroke_betas_by_sex);
  if(name=="comorbidity$ln_h_stroke_betas_by_sex") READ_R_MATRIX(value,input.comorbidity.ln_h_stroke_betas_by_sex);
  if(name=="comorbidity$p_stroke_death") {input.comorbidity.p_stroke_death=value[0]; return(0);}
  if(name=="comorbidity$logit_p_hf_betas_by_sex") READ_R_MATRIX(value,input.comorbidity.logit_p_hf_betas_by_sex);
  if(name=="comorbidity$ln_h_hf_betas_by_sex") READ_R_MATRIX(value,input.comorbidity.ln_h_hf_betas_by_sex);
  
#ifdef PROJECT_ROC16
  if(name=="project_specific$ROC16_biomarker_noise_sd") {input.project_specific.ROC16_biomarker_noise_sd=value[0]; return(0);}
  if(name=="project_specific$ROC16_biomarker_threshold") {input.project_specific.ROC16_biomarker_threshold=value[0]; return(0);}
  if(name=="project_specific$ROC16_treatment_RR") {input.project_specific.ROC16_treatment_RR=value[0]; return(0);}
  if(name=="project_specific$ROC16_biomarker_cost") {input.project_specific.ROC16_biomarker_cost=value[0]; return(0);}
  if(name=="project_specific$ROC16_treatment_cost") {input.project_specific.ROC16_treatment_cost=value[0]; return(0);}
#endif
  return(ERR_INCORRECT_INPUT_VAR);
}









// [[Rcpp::export]]
double get_sample_output(int year, int sex)
{
 return input.agent.p_bgd_by_sex[year][(int)sex];
}







/////////////////////////////////////////////////////////////////AGENT/////////////////////////////////////



struct agent
{
  long id;
  double local_time;
  bool alive;
  bool sex;
  
  double age_at_creation;
  double time_at_creation;
  
  double height;
  double weight;
  double weight_LPT;
  
  int smoking_status;   //0:not smoking, positive: current smoking rate (py per year), note that ex smoker status os determined also by pack_years
  double pack_years;
  double smoking_status_LPT;
  
  double fev1;
  double fev1_slope;  //fixed component of rate of decline;
  double fev1_slope_t;  //time-dependent component of FEV1 decline;
  double lung_function_LPT;
  int gold;
  int local_time_at_COPD;
  double _pred_fev1;
  
  double ln_exac_rate_intercept;   //backround rate of exacerbation (intercept only);
  double logit_exac_severity_intercept;   //backround severity of exacerbation (intercept only);
  
  int cumul_exac[3];    //0:mild, 1:moderate, 2:severe;
  double cumul_exac_time[3];
  double exac_LPT;  //the last time cumul exacerbation time was processed;
  int exac_status;    //current exacerbation status 0: no exacerbation, in 1: mild, 2:moderate, 3:severe exacerbation
  
  double symptom_score;
  
  double last_doctor_visit_time;
  int last_doctor_visit_type;
    
  int medication_status;    //0:on no med, positive values different classes (bitwise flags)
  double medication_LPT; 
  
  double cumul_cost;
  double cumul_qaly;
  double payoffs_LPT;
  
  int n_mi;
  double time_since_last_mi;
  int n_stroke;
  double time_since_last_stroke;
  double hf_status;   //0: no hf, + can indicate levels (for now: 1)
  double time_since_hf;
  
  double tte;
  int event; //carries the last event;
  
  double p_COPD;  //Not used in the model; here to facilitate calibration on incidence;
  
#ifdef PROJECT_ROC16
  double ROC16_exac_rate;
  double ROC16_treatment_status;
  double ROC16_biomarker;
#endif
  
};






agent *agent_stack;
long agent_stack_pointer;

agent smith;


/*
// [[Rcpp::export]]
int Cset_agent_var(long id, std::string name, NumericVector value)    //Imports the agent from 
{
   if(name=="local_time") {agent_stack[id].local_time=value[0]; return(0);}
   if(name=="alive") {agent_stack[id].alive=(value[0]!=0); return(0);}
   if(name=="sex") {agent_stack[id].sex=(value[0]!=0); return(0);}
   if(name=="dob") {agent_stack[id].dob=value[0]; return(0);}
   if(name=="age_at_creation") {agent_stack[id].age_at_creation=value[0]; return(0);}
   
   if(name=="event") {agent_stack[id].event=value[0]; return(0);}
   if(name=="tte") {agent_stack[id].tte=value[0]; return(0);}
    
   return(-1);
}
*/




List get_agent(agent *ag)
{
  List out=Rcpp::List::create(
      Rcpp::Named("id")=(*ag).id,
      Rcpp::Named("local_time")=(*ag).local_time,
      Rcpp::Named("alive")=(*ag).alive,
      Rcpp::Named("sex")=(int)(*ag).sex,
      
      Rcpp::Named("height")=(*ag).height,
      Rcpp::Named("weight")=(*ag).weight,
      
      Rcpp::Named("age_at_creation")=(*ag).age_at_creation,
      Rcpp::Named("time_at_creation")=(*ag).time_at_creation,
      
      Rcpp::Named("smoking_status")=(*ag).smoking_status,
      Rcpp::Named("pack_years")=(*ag).pack_years,
            
      Rcpp::Named("fev1")=(*ag).fev1,
      Rcpp::Named("fev1_slope")=(*ag).fev1_slope,
      Rcpp::Named("fev1_slope_t")=(*ag).fev1_slope_t,
      
      Rcpp::Named("exac_status")=(*ag).exac_status,
      Rcpp::Named("ln_exac_rate_intercept")=(*ag).ln_exac_rate_intercept,
      Rcpp::Named("logit_exac_severity_intercept")=(*ag).logit_exac_severity_intercept,
      
      Rcpp::Named("cumul_exac0")=(*ag).cumul_exac[0],
      Rcpp::Named("cumul_exac1")=(*ag).cumul_exac[1],
      Rcpp::Named("cumul_exac2")=(*ag).cumul_exac[2]
        
      );
      
    out["gold"]=(*ag).gold;
    out["local_time_at_COPD"]=(*ag).local_time_at_COPD;
  
    out["cumul_cost"]=(*ag).cumul_cost;
    out["cumul_qaly"]=(*ag).cumul_qaly;          
    out["tte"]=(*ag).tte;
    out["event"]=(*ag).event;
    out["symptom_score"]=(*ag).symptom_score;
    out["last_doctor_visit_time"]=(*ag).last_doctor_visit_time;
    out["last_doctor_visit_type"]=(*ag).last_doctor_visit_type;
    out["medication_status"]=(*ag).medication_status;
    
    out["n_mi"]=(*ag).n_mi;
    out["n_stroke"]=(*ag).n_stroke;
    
    out["p_COPD"]=(*ag).p_COPD;
    
    return out;
}


//This is a generic function as both agent_stack and event_stack are arrays of agents;
List get_agent(int id, agent agent_pointer[])
{
  return(get_agent(&agent_pointer[id]));
}



// [[Rcpp::export]]
List Cget_agent(long id)
{
 return(get_agent(id,agent_stack));
}



// [[Rcpp::export]]
List Cget_smith()
{
 return(get_agent(&smith));
}









#define CALC_MI_ODDS(ag) (exp(input.comorbidity.logit_p_mi_betas_by_sex[0][(*ag).sex]\
                      +input.comorbidity.logit_p_mi_betas_by_sex[1][(*ag).sex]*(*ag).age_at_creation\
                      +input.comorbidity.logit_p_mi_betas_by_sex[2][(*ag).sex]*(*ag).age_at_creation*(*ag).age_at_creation\
                      +input.comorbidity.logit_p_mi_betas_by_sex[3][(*ag).sex]*(*ag).pack_years\
                      +input.comorbidity.logit_p_mi_betas_by_sex[4][(*ag).sex]*(*ag).smoking_status\
                      +input.comorbidity.logit_p_mi_betas_by_sex[5][(*ag).sex]*calendar_time\
                      +input.comorbidity.logit_p_mi_betas_by_sex[6][(*ag).sex]*(*ag).weight/pow((*ag).height,2)\
                      +input.comorbidity.logit_p_mi_betas_by_sex[7][(*ag).sex]*(*ag).gold));\
                      
                    
#define CALC_PRED_FEV1(ag) (input.lung_function.pred_fev1_betas_by_sex[0][(*ag).sex]\
                    +input.lung_function.pred_fev1_betas_by_sex[1][(*ag).sex]*((*ag).age_at_creation+(*ag).local_time)  \
                    +input.lung_function.pred_fev1_betas_by_sex[2][(*ag).sex]*(*ag).height)  \
                    


agent *create_agent(agent *ag,int id)
{
  double _bvn[2]; //being used for joint estimation in multiple locations;
  
  (*ag).id=id;
  (*ag).alive=1;
  (*ag).local_time=0;
  (*ag).time_at_creation=calendar_time;
  
  (*ag).sex=rand_unif()<input.agent.p_female;
  
  double r=rand_unif();
  double cum_p=0;
  if(id<settings.n_base_agents) //the first n_base_agent cases are prevalent cases; the rest are incident ones;
    for(int i=input.global_parameters.age0;i<111;i++)
    {
      cum_p=cum_p+input.agent.p_prevalence_age[i];
      if(r<cum_p) {(*ag).age_at_creation=i+rand_unif(); break;}
    }
  else
    for(int i=input.global_parameters.age0;i<111;i++)
    {
      cum_p=cum_p+input.agent.p_incidence_age[i];
      if(r<cum_p) {(*ag).age_at_creation=i; break;}
    }
 
  rbvnorm(input.agent.height_weight_rho,_bvn);
  (*ag).height=_bvn[0]*input.agent.height_0_sd
              +input.agent.height_0_betas[0]
              +input.agent.height_0_betas[1]*(*ag).sex
              +input.agent.height_0_betas[2]*(*ag).age_at_creation
              +input.agent.height_0_betas[3]*(*ag).age_at_creation*(*ag).age_at_creation
              +input.agent.height_0_betas[4]*(*ag).age_at_creation*(*ag).sex;
              
  (*ag).weight=_bvn[1]*input.agent.weight_0_sd
              +input.agent.weight_0_betas[0]
              +input.agent.weight_0_betas[1]*(*ag).sex
              +input.agent.weight_0_betas[2]*(*ag).age_at_creation
              +input.agent.weight_0_betas[3]*(*ag).age_at_creation*(*ag).age_at_creation
              +input.agent.weight_0_betas[4]*(*ag).age_at_creation*(*ag).sex
              +input.agent.weight_0_betas[5]*(*ag).height
              +input.agent.weight_0_betas[6]*calendar_time;
  
  //smoking
  bool ever_smoker=false;
  
  double odds1=exp(input.smoking.logit_p_current_smoker_0_betas[0]
    +input.smoking.logit_p_current_smoker_0_betas[1]*(*ag).sex
    +input.smoking.logit_p_current_smoker_0_betas[2]*(*ag).age_at_creation
    +input.smoking.logit_p_current_smoker_0_betas[3]*pow((*ag).age_at_creation,2)
    +input.smoking.logit_p_current_smoker_0_betas[4]*(*ag).age_at_creation*(*ag).sex
    +input.smoking.logit_p_current_smoker_0_betas[5]*pow((*ag).age_at_creation,2)*(*ag).sex
    +input.smoking.logit_p_current_smoker_0_betas[6]*calendar_time
    );
  
  if(rand_unif()<odds1/(1+odds1)) //ever smoker
  {
    (*ag).smoking_status=1;
    ever_smoker=true;
  }
  else
  {
    (*ag).smoking_status=0;
    double odds2=exp(input.smoking.logit_p_ever_smoker_con_not_current_0_betas[0]
    +input.smoking.logit_p_ever_smoker_con_not_current_0_betas[1]*(*ag).sex
    +input.smoking.logit_p_ever_smoker_con_not_current_0_betas[2]*(*ag).age_at_creation
    +input.smoking.logit_p_ever_smoker_con_not_current_0_betas[3]*pow((*ag).age_at_creation,2)
    +input.smoking.logit_p_ever_smoker_con_not_current_0_betas[4]*(*ag).age_at_creation*(*ag).sex
    +input.smoking.logit_p_ever_smoker_con_not_current_0_betas[5]*pow((*ag).age_at_creation,2)*(*ag).sex
    +input.smoking.logit_p_ever_smoker_con_not_current_0_betas[6]*calendar_time
    );
    
    if(rand_unif()<odds2/(1+odds2))
      (*ag).pack_years=0;
    else ever_smoker=true;
  }
  if(ever_smoker)
  {
    double temp=input.smoking.pack_years_0_betas[0]
      +input.smoking.pack_years_0_betas[1]*(*ag).sex
      +input.smoking.pack_years_0_betas[2]*((*ag).age_at_creation+(*ag).local_time)
      +input.smoking.pack_years_0_betas[3]*((*ag).time_at_creation+(*ag).local_time)
      +input.smoking.pack_years_0_betas[4]*(*ag).smoking_status;
      
    (*ag).pack_years=temp+rand_norm()*input.smoking.pack_years_0_sd;
  }
  
    
  (*ag).smoking_status_LPT=0;
  
                     
  //exacerbation;
  rbvnorm(input.exacerbation.rate_severity_intercept_rho,_bvn);
  (*ag).ln_exac_rate_intercept=_bvn[0]*input.exacerbation.ln_rate_intercept_sd;
  (*ag).logit_exac_severity_intercept=_bvn[1]*input.exacerbation.logit_severity_intercept_sd;
  
  (*ag).cumul_exac[0]=0;
  (*ag).cumul_exac[1]=0;
  (*ag).cumul_exac[2]=0;
  (*ag).cumul_exac_time[0]=0;
  (*ag).cumul_exac_time[1]=0;
  (*ag).cumul_exac_time[2]=0;
  (*ag).exac_status=0;
  (*ag).exac_LPT=0;


  //COPD;
  double COPD_odds=exp(input.COPD.logit_p_COPD_betas_by_sex[0][(*ag).sex]
                      +input.COPD.logit_p_COPD_betas_by_sex[1][(*ag).sex]*(*ag).age_at_creation
                      +input.COPD.logit_p_COPD_betas_by_sex[2][(*ag).sex]*(*ag).age_at_creation*(*ag).age_at_creation
                      +input.COPD.logit_p_COPD_betas_by_sex[3][(*ag).sex]*(*ag).pack_years
                      +input.COPD.logit_p_COPD_betas_by_sex[4][(*ag).sex]*(*ag).smoking_status
                      +input.COPD.logit_p_COPD_betas_by_sex[5][(*ag).sex]*calendar_time)
                      //+input.COPD.logit_p_COPD_betas_by_sex[7]*(*ag).asthma
              ;            
  
  (*ag).p_COPD=COPD_odds/(1+COPD_odds);
            
  if(rand_unif()<COPD_odds/(1+COPD_odds))
  {
    (*ag).fev1=input.lung_function.fev1_0_prev_betas_by_sex[0][(*ag).sex]
    +input.lung_function.fev1_0_prev_betas_by_sex[1][(*ag).sex]*((*ag).age_at_creation+(*ag).local_time)
    +input.lung_function.fev1_0_prev_betas_by_sex[2][(*ag).sex]*(*ag).height
    +input.lung_function.fev1_0_prev_betas_by_sex[3][(*ag).sex]*(*ag).pack_years
    +rand_norm()*input.lung_function.fev1_0_prev_sd_by_sex[(*ag).sex];
    
    double pred_fev1=CALC_PRED_FEV1(ag);
    (*ag)._pred_fev1=pred_fev1;
    
    if ((*ag).fev1/pred_fev1<0.3) (*ag).gold=4;
    else 
      if ((*ag).fev1/pred_fev1<0.5) (*ag).gold=3;
      else
        if ((*ag).fev1/pred_fev1<0.8) (*ag).gold=2;
        else (*ag).gold=1;
  } 
  else 
  {
    (*ag).gold=0;
    (*ag).fev1=0;
  }
  
  
  //lung function;
  (*ag).lung_function_LPT=0;
  
  
  
//medication
  (*ag).medication_status=0;
  (*ag).medication_LPT=0;
  
//comorbidity
//mi
  double mi_odds=CALC_MI_ODDS(ag);

  if(rand_unif()<mi_odds/(1+mi_odds))
  {
    (*ag).n_mi=1;
  }
  else
  {
    (*ag).n_mi=0;
  }

  
  //stroke 
  double stroke_odds=exp(input.comorbidity.logit_p_stroke_betas_by_sex[0][(*ag).sex]
                      +input.comorbidity.logit_p_stroke_betas_by_sex[1][(*ag).sex]*(*ag).age_at_creation
                      +input.comorbidity.logit_p_stroke_betas_by_sex[2][(*ag).sex]*(*ag).age_at_creation*(*ag).age_at_creation
                      +input.comorbidity.logit_p_stroke_betas_by_sex[3][(*ag).sex]*(*ag).pack_years
                      +input.comorbidity.logit_p_stroke_betas_by_sex[4][(*ag).sex]*(*ag).smoking_status
                      +input.comorbidity.logit_p_stroke_betas_by_sex[5][(*ag).sex]*calendar_time
                      +input.comorbidity.logit_p_stroke_betas_by_sex[6][(*ag).sex]*(*ag).weight/pow((*ag).height,2)
                      +input.comorbidity.logit_p_stroke_betas_by_sex[7][(*ag).sex]*(*ag).gold
                      +input.comorbidity.logit_p_stroke_betas_by_sex[8][(*ag).sex]*((*ag).n_mi>0)
                      +input.comorbidity.logit_p_stroke_betas_by_sex[9][(*ag).sex]*((*ag).n_mi)
              );
  if(rand_unif()<stroke_odds/(1+stroke_odds))
  {
    (*ag).n_stroke=1;
  }
  else
  {
    (*ag).n_stroke=0;
  }
  

//hf 
  double hf_odds=exp(input.comorbidity.logit_p_hf_betas_by_sex[0][(*ag).sex]
                      +input.comorbidity.logit_p_hf_betas_by_sex[1][(*ag).sex]*(*ag).age_at_creation
                      +input.comorbidity.logit_p_hf_betas_by_sex[2][(*ag).sex]*(*ag).age_at_creation*(*ag).age_at_creation
                      +input.comorbidity.logit_p_hf_betas_by_sex[3][(*ag).sex]*(*ag).pack_years
                      +input.comorbidity.logit_p_hf_betas_by_sex[4][(*ag).sex]*(*ag).smoking_status
                      +input.comorbidity.logit_p_hf_betas_by_sex[5][(*ag).sex]*calendar_time
                      +input.comorbidity.logit_p_hf_betas_by_sex[6][(*ag).sex]*(*ag).weight/pow((*ag).height,2)
                      +input.comorbidity.logit_p_hf_betas_by_sex[7][(*ag).sex]*(*ag).gold
                      +input.comorbidity.logit_p_hf_betas_by_sex[8][(*ag).sex]*((*ag).n_mi>0)
                      +input.comorbidity.logit_p_hf_betas_by_sex[9][(*ag).sex]*((*ag).n_mi)
                      +input.comorbidity.logit_p_hf_betas_by_sex[10][(*ag).sex]*((*ag).n_stroke>0)
                      +input.comorbidity.logit_p_hf_betas_by_sex[11][(*ag).sex]*((*ag).n_stroke)
              );
  if(rand_unif()<hf_odds/(1+hf_odds))
  {
    (*ag).hf_status=1;
  }
  else
  {
    (*ag).hf_status=0;
  }
  



//payoffs;
  (*ag).cumul_cost=0;
  (*ag).cumul_qaly=0;
  (*ag).payoffs_LPT=0;

#ifdef PROJECT_ROC16
  (*ag).ROC16_treatment_status=0;
  (*ag).ROC16_biomarker=-1;
#endif
  
  return(ag);
}










// [[Rcpp::export]]
int Ccreate_agents()
{
  if(agent_stack==NULL) return(-1);
  for(int i=0;i<settings.agent_stack_size;i++)
  {
    create_agent(&agent_stack[i],i);
  }
  
  return(0);
}






/////////////////////////////////////////////////////////////////////////OUTPUT/////////////////////////////////////////////////

struct output
{
  int n_agents;
  double cumul_time;    //End variable by nature;
  int n_deaths;         //End variable by nature.
  int n_COPD;
  double total_pack_years;    //END  because agent records
  int total_exac[3];    //0:mild, 1:moderae, 2:severe;    END because agent records
  double total_exac_time[3];  //END because agent records
  
  int total_doctor_visit[2];  //0: GP, 1:SP
   
  double total_cost;    //END because agent records
  double total_qaly;  //END because agent records
#ifdef PROJECT_ROC16
  int n_eligible;
  int n_treated;
#endif
} output;




void reset_output()
{
  output.n_agents=0;
  output.cumul_time=0;
  output.n_deaths=0;
  output.n_COPD=0;
  output.total_pack_years=0;
  output.total_exac[0]=0;output.total_exac[1]=0;output.total_exac[2]=0;
  output.total_exac_time[0]=0;output.total_exac_time[1]=0;output.total_exac_time[2]=0;
  output.total_doctor_visit[0]=0;output.total_doctor_visit[1]=0;
  output.total_cost=0;
  output.total_qaly=0;
#ifdef PROJECT_ROC16
  output.n_eligible=0;
  output.n_treated=0;
#endif
}


// [[Rcpp::export]]
List Cget_output()
{
 return Rcpp::List::create(
      Rcpp::Named("n_agents")=output.n_agents,
      Rcpp::Named("cumul_time")=output.cumul_time,
      Rcpp::Named("n_deaths")=output.n_deaths,
      Rcpp::Named("n_COPD")=output.n_COPD,
      Rcpp::Named("total_exac")=AS_VECTOR_INT(output.total_exac),
      Rcpp::Named("total_exac_time")=AS_VECTOR_DOUBLE(output.total_exac_time),
      Rcpp::Named("total_pack_years")=output.total_pack_years,
      Rcpp::Named("total_doctor_visit")=AS_VECTOR_INT(output.total_doctor_visit),
      Rcpp::Named("total_cost")=output.total_cost,
      Rcpp::Named("total_qaly")=output.total_qaly
#ifdef PROJECT_ROC16
      ,
      Rcpp::Named("n_eligible")=output.n_eligible,
      Rcpp::Named("n_treated")=output.n_treated
#endif
    );
}








#ifdef OUTPUT_EX

struct output_ex
{
  int n_alive_by_ctime_sex[1000][2];      //number of folks alive at each fixed time;
  int n_smoking_status_by_ctime[1000][3];
  int n_alive_by_ctime_age[1000][111];
  int n_current_smoker_by_ctime_sex[1000][2];
  double cumul_cost_ctime[1000];
  double cumul_qaly_ctime[1000];
  double sum_fev1_ltime[1000];
  double cumul_time_by_smoking_status[3];
  double sum_time_by_ctime_sex[100][2];
  double sum_time_by_age_sex[111][2];
  
#if OUTPUT_EX > 1 
  double cumul_non_COPD_time;
  double sum_p_COPD_by_ctime_sex[1000][2];
  double sum_pack_years_by_ctime_sex[1000][2];
  double sum_age_by_ctime_sex[1000][2];
  int n_death_by_age_sex[111][2];
  int n_alive_by_age_sex[111][2];
#endif
  
#if (OUTPUT_EX & OUTPUT_EX_COPD) > 0 
  int n_COPD_by_ctime_sex[1000][2];
  int n_COPD_by_ctime_age[100][111];
  int n_inc_COPD_by_ctime_age[100][111];
  int n_COPD_by_ctime_severity[100][5]; //no COPD to GOLD 4;
#endif
  
#if (OUTPUT_EX & OUTPUT_EX_EXACERBATION) > 0 
  int n_exac_by_ctime_age[100][111];
  int n_exac_by_ctime_severity[100][3];
#endif
  
#if (OUTPUT_EX & OUTPUT_EX_COMORBIDITY) > 0 
  int n_mi;
  int n_incident_mi;
  int n_mi_by_age_sex[111][2];
  int n_mi_by_ctime_sex[1000][2];
  double sum_p_mi_by_ctime_sex[1000][2];
  
    
  int n_stroke;
  int n_incident_stroke;
  int n_stroke_by_age_sex[111][2];
  int n_stroke_by_ctime_sex[1000][2];
    
  int n_hf;                 //total number of agents who had HF at some point;
  int n_incident_hf;        //total number of agents who had incident HF (no HF at birth)
  int n_hf_by_age_sex[111][2];  //NOTE: unlike Mi and stroke, HF is a prevalent disease (like COPD), so this is a carry-forward quantity
  int n_hf_by_ctime_sex[1000][2]; //same as above
#endif

#if (OUTPUT_EX & OUTPUT_EX_BIOMETRICS) > 0 
  double sum_weight_by_ctime_sex[1000][2];
#endif

#if (OUTPUT_EX & OUTPUT_EX_MEDICATION) > 0 
  double medication_time_by_class[N_MED_CLASS];
  double n_exac_by_medication_class[N_MED_CLASS][3];
#endif

} output_ex;
#endif



void reset_output_ex()
{
#ifdef OUTPUT_EX
  char *x=reinterpret_cast <char *>(&output_ex);
  for(unsigned i=0;i<sizeof(output_ex);i++)
    x[i]=0;
#endif
}


// [[Rcpp::export]]
List Cget_output_ex()
{
 List out=Rcpp::List::create(
#ifdef OUTPUT_EX
      Rcpp::Named("n_alive_by_ctime_sex")=AS_MATRIX_INT_SIZE(output_ex.n_alive_by_ctime_sex,input.global_parameters.time_horizon),
      Rcpp::Named("n_alive_by_ctime_age")=AS_MATRIX_INT_SIZE(output_ex.n_alive_by_ctime_age,input.global_parameters.time_horizon),
      Rcpp::Named("n_smoking_status_by_ctime")=AS_MATRIX_INT_SIZE(output_ex.n_smoking_status_by_ctime,input.global_parameters.time_horizon),
      Rcpp::Named("n_current_smoker_by_ctime_sex")=AS_MATRIX_INT_SIZE(output_ex.n_current_smoker_by_ctime_sex,input.global_parameters.time_horizon),
      Rcpp::Named("cumul_cost_ctime")=AS_VECTOR_DOUBLE_SIZE(output_ex.cumul_cost_ctime,input.global_parameters.time_horizon),
      Rcpp::Named("cumul_qaly_ctime")=AS_VECTOR_DOUBLE_SIZE(output_ex.cumul_qaly_ctime,input.global_parameters.time_horizon),
      Rcpp::Named("sum_fev1_ltime")=AS_VECTOR_DOUBLE_SIZE(output_ex.sum_fev1_ltime,input.global_parameters.time_horizon),
      Rcpp::Named("cumul_time_by_smoking_status")=AS_VECTOR_DOUBLE(output_ex.cumul_time_by_smoking_status),
      Rcpp::Named("cumul_non_COPD_time")=output_ex.cumul_non_COPD_time,
      Rcpp::Named("sum_p_COPD_by_ctime_sex")=AS_MATRIX_DOUBLE_SIZE(output_ex.sum_p_COPD_by_ctime_sex,input.global_parameters.time_horizon),
      Rcpp::Named("sum_pack_years_by_ctime_sex")=AS_MATRIX_DOUBLE_SIZE(output_ex.sum_pack_years_by_ctime_sex,input.global_parameters.time_horizon),
      Rcpp::Named("sum_age_by_ctime_sex")=AS_MATRIX_DOUBLE_SIZE(output_ex.sum_age_by_ctime_sex,input.global_parameters.time_horizon),
      Rcpp::Named("n_death_by_age_sex")=AS_MATRIX_INT(output_ex.n_death_by_age_sex),
      Rcpp::Named("n_alive_by_age_sex")=AS_MATRIX_INT(output_ex.n_alive_by_age_sex),
      Rcpp::Named("sum_time_by_ctime_sex")=AS_MATRIX_DOUBLE_SIZE(output_ex.sum_time_by_ctime_sex,input.global_parameters.time_horizon),
      Rcpp::Named("sum_time_by_age_sex")=AS_MATRIX_DOUBLE(output_ex.sum_time_by_age_sex)
#endif
#if (OUTPUT_EX & OUTPUT_EX_BIOMETRICS) > 0 
  ,Rcpp::Named("sum_weight_by_ctime_sex")=AS_MATRIX_DOUBLE_SIZE(output_ex.sum_weight_by_ctime_sex,input.global_parameters.time_horizon)
#endif
    );

  
#if (OUTPUT_EX & OUTPUT_EX_COPD)>0
  out["n_COPD_by_ctime_sex"]=AS_MATRIX_INT_SIZE(output_ex.n_COPD_by_ctime_sex,input.global_parameters.time_horizon),
  out["n_COPD_by_ctime_age"]=AS_MATRIX_INT_SIZE(output_ex.n_COPD_by_ctime_age,input.global_parameters.time_horizon),
  out["n_inc_COPD_by_ctime_age"]=AS_MATRIX_INT_SIZE(output_ex.n_inc_COPD_by_ctime_age,input.global_parameters.time_horizon),
  out["n_COPD_by_ctime_severity"]=AS_MATRIX_INT_SIZE(output_ex.n_COPD_by_ctime_severity,input.global_parameters.time_horizon),
#endif
  
  
#if (OUTPUT_EX & OUTPUT_EX_EXACERBATION)>0
  out["n_exac_by_ctime_age"]=AS_MATRIX_INT_SIZE(output_ex.n_exac_by_ctime_age,input.global_parameters.time_horizon);
  out["n_exac_by_ctime_severity"]=AS_MATRIX_INT_SIZE(output_ex.n_exac_by_ctime_severity,input.global_parameters.time_horizon);
#endif
  
  
#if (OUTPUT_EX & OUTPUT_EX_COMORBIDITY)>0
  out["n_mi"]=output_ex.n_mi;
  out["n_incident_mi"]=output_ex.n_incident_mi;
  out["n_mi_by_age_sex"]=AS_MATRIX_INT(output_ex.n_mi_by_age_sex);
  out["n_mi_by_ctime_sex"]=AS_MATRIX_INT_SIZE(output_ex.n_mi_by_ctime_sex,input.global_parameters.time_horizon);
  out["sum_p_mi_by_ctime_sex"]=AS_MATRIX_DOUBLE_SIZE(output_ex.sum_p_mi_by_ctime_sex,input.global_parameters.time_horizon);
        
  out["n_stroke"]=output_ex.n_stroke;
  out["n_incident_stroke"]=output_ex.n_incident_stroke;
  out["n_stroke_by_age_sex"]=AS_MATRIX_INT(output_ex.n_stroke_by_age_sex);
  out["n_stroke_by_ctime_sex"]=AS_MATRIX_INT_SIZE(output_ex.n_stroke_by_ctime_sex,input.global_parameters.time_horizon);
  out["n_hf"]=output_ex.n_hf;
  out["n_incident_hf"]=output_ex.n_incident_hf;
  out["n_hf_by_age_sex"]=AS_MATRIX_INT(output_ex.n_hf_by_age_sex);
  out["n_hf_by_ctime_sex"]=AS_MATRIX_INT_SIZE(output_ex.n_hf_by_ctime_sex,input.global_parameters.time_horizon);
#endif

#if (OUTPUT_EX & OUTPUT_EX_MEDICATION)>0
  out["medication_time_by_class"]=AS_VECTOR_DOUBLE(output_ex.medication_time_by_class);
  out["n_exac_by_medication_class"]=AS_MATRIX_DOUBLE(output_ex.n_exac_by_medication_class);
#endif

  return(out);
}



//This function must run ONLY on start and fixed events; any other place and will mess up!
void update_output_ex(agent *ag)
{
#ifdef OUTPUT_EX
  int time=floor((*ag).local_time+(*ag).time_at_creation);
  int local_time=floor((*ag).local_time);
  
  //if(time>=(*ag).time_at_creation)
  {
    int age=floor((*ag).age_at_creation+(*ag).local_time);
    output_ex.n_alive_by_ctime_age[time][age]+=1;
    output_ex.n_alive_by_ctime_sex[time][(*ag).sex]+=1;
    output_ex.n_alive_by_age_sex[age][(*ag).sex]+=1;
    if((*ag).smoking_status==1)
    {
      output_ex.n_smoking_status_by_ctime[time][1]+=1;
      output_ex.n_current_smoker_by_ctime_sex[time][(*ag).sex]+=1;
    }
    else
      if((*ag).pack_years>0)
        output_ex.n_smoking_status_by_ctime[time][2]+=1;
      else
        output_ex.n_smoking_status_by_ctime[time][0]+=1;
      
    output_ex.cumul_cost_ctime[time]+=(*ag).cumul_cost;
    output_ex.cumul_qaly_ctime[time]+=(*ag).cumul_qaly;
  
    output_ex.sum_fev1_ltime[local_time]+=(*ag).fev1;
  
    double odds=exp(input.COPD.logit_p_COPD_betas_by_sex[0][(*ag).sex]
      +input.COPD.logit_p_COPD_betas_by_sex[1][(*ag).sex]*((*ag).age_at_creation+(*ag).local_time)
      +input.COPD.logit_p_COPD_betas_by_sex[2][(*ag).sex]*pow((*ag).age_at_creation+(*ag).local_time,2)
      +input.COPD.logit_p_COPD_betas_by_sex[3][(*ag).sex]*(*ag).pack_years
      +input.COPD.logit_p_COPD_betas_by_sex[4][(*ag).sex]*(*ag).smoking_status
      +input.COPD.logit_p_COPD_betas_by_sex[5][(*ag).sex]*(calendar_time+(*ag).local_time)
      );
    output_ex.sum_p_COPD_by_ctime_sex[time][(*ag).sex]+=odds/(1+odds);
    output_ex.sum_pack_years_by_ctime_sex[time][(*ag).sex]+=(*ag).pack_years;
    output_ex.sum_age_by_ctime_sex[time][(*ag).sex]+=(*ag).age_at_creation+(*ag).local_time;

#if (OUTPUT_EX & OUTPUT_EX_BIOMETRICS)>0
  output_ex.sum_weight_by_ctime_sex[time][(*ag).sex]+=(*ag).weight;
#endif
  
#if (OUTPUT_EX & OUTPUT_EX_COPD)>0
  output_ex.n_COPD_by_ctime_sex[time][(*ag).sex]+=((*ag).gold>0)*1;
  output_ex.n_COPD_by_ctime_age[time][age]+=((*ag).gold>0)*1;
  output_ex.n_COPD_by_ctime_severity[time][((*ag).gold)]+=1;
#endif

#if (OUTPUT_EX & OUTPUT_EX_COMORBIDITY)>0
    double mi_odds=CALC_MI_ODDS(ag);
    output_ex.sum_p_mi_by_ctime_sex[time][(*ag).sex]+=mi_odds/(1+mi_odds);
    if((*ag).hf_status>0)
    {
      output_ex.n_hf_by_age_sex[age][(*ag).sex]++;
      output_ex.n_hf_by_ctime_sex[time][(*ag).sex]++;
    }
#endif
  }
#endif
}







/////////////////////////////////////////////////////////////////////////LPTs////////////////////////////////////////////////////////////////////////////////



void lung_function_LPT(agent *ag)
{
  if((*ag).gold==0) 
  {
    //We are not currently modeling lung funciton in non-COPD patients. Everything starts when gold>0;
  }
  else  //apply LHS equations
  {
    double dt=(*ag).local_time-(*ag).lung_function_LPT;
    (*ag).fev1=(*ag).fev1 + (*ag).fev1_slope*dt + 2*(*ag).fev1_slope_t*(*ag).local_time*dt + (*ag).fev1_slope_t*dt*dt;
    
    double pred_fev1=CALC_PRED_FEV1(ag);
    (*ag)._pred_fev1=pred_fev1;
    if ((*ag).fev1/pred_fev1<0.3) (*ag).gold=4;
    else 
      if ((*ag).fev1/pred_fev1<0.5) (*ag).gold=3;
      else
        if ((*ag).fev1/pred_fev1<0.8) (*ag).gold=2;
        else (*ag).gold=1;
  }
  (*ag).lung_function_LPT=(*ag).local_time;
}


void smoking_LPT(agent *ag)
{
#ifdef OUTPUT_EX
  if((*ag).smoking_status==0) output_ex.cumul_time_by_smoking_status[0]+=(*ag).local_time-(*ag).smoking_status_LPT;
  else 
    if((*ag).pack_years>0) output_ex.cumul_time_by_smoking_status[2]+=(*ag).local_time-(*ag).smoking_status_LPT;
    else output_ex.cumul_time_by_smoking_status[1]+=(*ag).local_time-(*ag).smoking_status_LPT;
#endif

  (*ag).pack_years+=(*ag).smoking_status*((*ag).local_time-(*ag).smoking_status_LPT);
  (*ag).smoking_status_LPT=(*ag).local_time;
}


void exacerbation_LPT(agent *ag)
{
  if((*ag).exac_status>0)
    (*ag).cumul_exac_time[(*ag).exac_status-1]+=(*ag).local_time-(*ag).exac_LPT;
  (*ag).exac_LPT=(*ag).local_time;
}


void payoffs_LPT(agent *ag)
{
  (*ag).cumul_cost+=input.cost.bg_cost_by_stage[(*ag).gold]*((*ag).local_time-(*ag).payoffs_LPT)/pow(1+input.global_parameters.discount_cost,(*ag).local_time+calendar_time);
  (*ag).cumul_qaly+=input.utility.bg_util_by_stage[(*ag).gold]*((*ag).local_time-(*ag).payoffs_LPT)/pow(1+input.global_parameters.discount_qaly,(*ag).local_time+calendar_time);
  (*ag).payoffs_LPT=(*ag).local_time;
}


void medication_LPT(agent *ag)
{
#if (OUTPUT_EX & OUTPUT_EX_MEDICATION) > 0
  for(int i=0;i<N_MED_CLASS;i++)
    if(((*ag).medication_status >> i) & 1)
    {
      output_ex.medication_time_by_class[i]+=((*ag).local_time-(*ag).medication_LPT);  
    }
  
#endif
  (*ag).medication_LPT=(*ag).local_time;
}





///////////////////////////////////////////////////////////////////EVENT/////////////////////////////////////////////////////////



enum events
{
  event_start=0,
  event_fixed=1,
  event_birthday=2,
  event_smoking_change=3,
  event_COPD=4,
  event_exacerbation=5,
  event_exacerbation_end=6,
  event_exacerbation_death=7,
  event_doctor_visit=8,
  event_medication_change=9,
  
  event_mi=10,
  event_stroke=11,
  event_hf=12,
  event_bgd=13,
  event_end=14
};
/*** R
 events<-c(
   event_start=0,
   event_fixed=1,
   event_birthday=2,
   event_smoking_change=3,
   event_COPD=4,
   event_exacerbation=5,
   event_exacerbation_end=6,
   event_exacerbation_death=7,
   event_doctor_visit=8,
   event_medication_change=9,
   
   event_mi=10,
   event_stroke=11,
   event_hf=12,
   
   event_bgd=13,
   event_end=14
  )
*/








agent *event_start_process(agent *ag)
{
#ifdef OUTPUT_EX
  update_output_ex(ag);
#endif
  return(ag);
}




agent *event_end_process(agent *ag)
{
  if((*ag).exac_status>0)
  {
    //NOTE: exacerbation timing is an LPT process and is treated separately. 
    (*ag).cumul_cost+=input.cost.exac_dcost[(*ag).exac_status-1]/pow(1+input.global_parameters.discount_cost,(*ag).local_time+calendar_time);
    (*ag).cumul_qaly+=input.utility.exac_dutil[(*ag).exac_status-1]/pow(1+input.global_parameters.discount_qaly,(*ag).local_time+calendar_time);
  }
  
  ++output.n_agents;
  output.n_COPD+=((*ag).gold>0)*1;
  output.cumul_time+=(*ag).local_time;
  output.n_deaths+=!(*ag).alive;
  
  lung_function_LPT(ag);
  smoking_LPT(ag);
  exacerbation_LPT(ag);
  payoffs_LPT(ag);
  medication_LPT(ag);
  
  output.total_pack_years+=(*ag).pack_years;
  output.total_exac[0]+=(*ag).cumul_exac[0];
  output.total_exac[1]+=(*ag).cumul_exac[1];
  output.total_exac[2]+=(*ag).cumul_exac[2];
  
  output.total_exac_time[0]+=(*ag).cumul_exac_time[0];
  output.total_exac_time[1]+=(*ag).cumul_exac_time[1];
  output.total_exac_time[2]+=(*ag).cumul_exac_time[2];
  
  output.total_cost+=(*ag).cumul_cost;
  output.total_qaly+=(*ag).cumul_qaly;
  
  
#ifdef OUTPUT_EX
//NO!!! We do not update many of output_ex stuff here. It might fall within the same calendar year of the last fixed event ad results in double counting. 
//If it falls after that still we ignore as it is a partially observed year.
#endif
#if OUTPUT_EX>1
  int age=floor((*ag).local_time+(*ag).age_at_creation);
  //Rprintf("age at death=%d\n",age);
  if((*ag).gold==0) output_ex.cumul_non_COPD_time+=(*ag).local_time;
  if((*ag).alive==false)  output_ex.n_death_by_age_sex[age][(*ag).sex]+=1;
  
  double time=(*ag).time_at_creation+(*ag).local_time;
  while(time>(*ag).time_at_creation)
  {
    int time_cut=floor(time);
    double delta=min(time-time_cut,time-(*ag).time_at_creation);
    if(delta==0) {time_cut-=1; delta=min(time-time_cut,time-(*ag).time_at_creation);}
    output_ex.sum_time_by_ctime_sex[time_cut][(*ag).sex]+=delta;
    time-=delta;
  }
  
  
  double _age=(*ag).age_at_creation+(*ag).local_time;
  while(_age>(*ag).age_at_creation)
  {
    int age_cut=floor(_age);
    double delta=min(_age-age_cut,_age-(*ag).age_at_creation);
    if(delta==0) {age_cut-=1; delta=min(_age-age_cut,_age-(*ag).age_at_creation);}
    output_ex.sum_time_by_age_sex[age_cut][(*ag).sex]+=delta;
    _age-=delta;
  }
  
#endif


#if (OUTPUT_EX & OUTPUT_EX_COMORBIDITY) > 0 
  output_ex.n_mi+=(*ag).n_mi;
  output_ex.n_stroke+=(*ag).n_stroke;
  output_ex.n_hf+=((*ag).hf_status>0);
#endif
  
  return(ag);
}









agent *event_stack;
int event_stack_pointer;






int push_event(agent *ag)
{
  if(event_stack_pointer==settings.event_stack_size) return(ERR_EVENT_STACK_FULL);
  if(settings.record_mode==record_mode_some_event)
  {
    int i;
    for(i=0;i<settings.n_events_to_record;i++)
    {
      if(settings.events_to_record[i]==(*ag).event)
      {
        event_stack[event_stack_pointer]=*ag;
        ++event_stack_pointer;
        return 0;
      }
    }
    return 0;
  }
   event_stack[event_stack_pointer]=*ag;
  ++event_stack_pointer;
  return(0);
}




// [[Rcpp::export]]
List Cget_event(int i)
{
 return(get_agent(i,event_stack));
}



// [[Rcpp::export]]
int Cget_n_events() //number of events, not n events themselves;
{
 return(event_stack_pointer);
}



// [[Rcpp::export]]
DataFrame Cget_agent_events(int id) //Returns ALLva events of an agent;
{
  DataFrame dfout;
  
  for(int i=0;i<event_stack_pointer;i++)
  {
    if(event_stack[i].id==id)
    {
      dfout.push_back(get_agent(i,event_stack));
    }  
  }
  return(dfout);
}




// [[Rcpp::export]]
DataFrame Cget_events_by_type(int event_type) //Returns all events of a given type for an agent;
{
  DataFrame dfout;
  
  for(int i=0;i<event_stack_pointer;i++)
  {
    if(event_stack[i].event==event_type)
    {
      dfout.push_back(get_agent(i,event_stack));
    }  
  }
  return(dfout);
}




// [[Rcpp::export]]
DataFrame Cget_all_events() //Returns all events from all agents;
{
  DataFrame dfout;
  
  for(int i=0;i<event_stack_pointer;i++)
  {
    dfout.push_back(get_agent(i,event_stack));
  }
  return(dfout);
}




// [[Rcpp::export]]
NumericMatrix Cget_all_events_matrix()
{
  NumericMatrix outm(event_stack_pointer,13);
  colnames(outm) = CharacterVector::create("id","local_time","sex", "time_at_creation", "age_at_creation", "pack_years","gold","event","FEV1","_pred_FEV1","smoking_status","ROC16_exac_rate","ROC16_biomarker");
  for(int i=0;i<event_stack_pointer;i++)
  {
    agent *ag=&event_stack[i];
    outm(i,0)=(*ag).id;
    outm(i,1)=(*ag).local_time;
    outm(i,2)=(*ag).sex;
    outm(i,3)=(*ag).time_at_creation;
    outm(i,4)=(*ag).age_at_creation;
    outm(i,5)=(*ag).pack_years;
    outm(i,6)=(*ag).gold;  
    outm(i,7)=(*ag).event;  
    outm(i,8)=(*ag).fev1;
    outm(i,9)=(*ag)._pred_fev1;
    outm(i,10)=(*ag).smoking_status;
    outm(i,11)=(*ag).ROC16_exac_rate;
    outm(i,12)=(*ag).ROC16_biomarker;
  }
  return(outm);
}




//////////////////////////////////////////////////////////////////EVENT_SMOKING////////////////////////////////////;
double event_smoking_change_tte(agent *ag)
{
  double rate;
  
  
  if((*ag).smoking_status==0)
  {
    rate=exp(input.smoking.ln_h_inc_betas[0]
        +input.smoking.ln_h_inc_betas[1]*(*ag).sex
        +input.smoking.ln_h_inc_betas[2]*((*ag).age_at_creation+(*ag).local_time)
        +input.smoking.ln_h_inc_betas[3]*pow((*ag).age_at_creation+(*ag).local_time,2)
        +input.smoking.ln_h_inc_betas[4]*(calendar_time+(*ag).local_time)); //TODO:should be cal+loc time??? 
  }
  else
  {
    rate=exp(input.smoking.ln_h_ces_betas[0]
        +input.smoking.ln_h_ces_betas[1]*(*ag).sex
        +input.smoking.ln_h_ces_betas[2]*((*ag).age_at_creation+(*ag).local_time)
        +input.smoking.ln_h_ces_betas[3]*pow((*ag).age_at_creation+(*ag).local_time,2)
        +input.smoking.ln_h_ces_betas[4]*(calendar_time+(*ag).local_time)); 
  }
  

  double tte=rand_exp()/rate;
  
  return(tte);
}





void event_smoking_change_process(agent *ag)
{
  smoking_LPT(ag);
  
  if((*ag).smoking_status==0)
    (*ag).smoking_status=1;
  else
    (*ag).smoking_status=0;
}









//////////////////////////////////////////////////////////////////EVENT_COPD////////////////////////////////////;
double event_COPD_tte(agent *ag)
{
  if((*ag).gold>0) return(HUGE_VAL);
  
  double rate=exp(input.COPD.ln_h_COPD_betas_by_sex[0][(*ag).sex]
                  +input.COPD.ln_h_COPD_betas_by_sex[1][(*ag).sex]*((*ag).age_at_creation+(*ag).local_time)
                  +input.COPD.ln_h_COPD_betas_by_sex[2][(*ag).sex]*pow((*ag).age_at_creation+(*ag).local_time,2)
                  +input.COPD.ln_h_COPD_betas_by_sex[3][(*ag).sex]*(*ag).pack_years
                  +input.COPD.ln_h_COPD_betas_by_sex[4][(*ag).sex]*(*ag).smoking_status
                  +input.COPD.ln_h_COPD_betas_by_sex[5][(*ag).sex]*(calendar_time+(*ag).local_time)
                  );
  
  double tte;
  if(rate==0) tte=HUGE_VAL; else tte=rand_exp()/rate;
  //return(HUGE_VAL);
  return(tte);
}





void event_COPD_process(agent *ag)
{
  (*ag).fev1=input.lung_function.fev1_0_inc_betas_by_sex[0][(*ag).sex]
            +input.lung_function.fev1_0_inc_betas_by_sex[1][(*ag).sex]*((*ag).age_at_creation+(*ag).local_time)
            +input.lung_function.fev1_0_inc_betas_by_sex[2][(*ag).sex]*(*ag).height
            +input.lung_function.fev1_0_inc_betas_by_sex[3][(*ag).sex]*(*ag).pack_years
            +rand_norm()*input.lung_function.fev1_0_inc_sd_by_sex[(*ag).sex];
            
  double pred_fev1=CALC_PRED_FEV1(ag);
  (*ag)._pred_fev1=pred_fev1;
  if ((*ag).fev1/pred_fev1<0.3) (*ag).gold=4;
  else 
    if ((*ag).fev1/pred_fev1<0.5) (*ag).gold=3;
    else
      if ((*ag).fev1/pred_fev1<0.8) (*ag).gold=2;
      else (*ag).gold=1;

  double temp[2];
  rbvnorm(input.lung_function.dfev1_re_rho,temp);
  (*ag).fev1_slope=input.lung_function.dfev1_betas[0]
                   +temp[0]*input.lung_function.dfev1_re_sds[0]
                   +input.lung_function.dfev1_betas[1]*(*ag).sex
                   +input.lung_function.dfev1_betas[2]*((*ag).age_at_creation+(*ag).local_time)
                   +input.lung_function.dfev1_betas[3]*(*ag).fev1
                   +input.lung_function.dfev1_betas[4]*(*ag).smoking_status;

  (*ag).fev1_slope_t=input.lung_function.dfev1_betas[5]+temp[1]*input.lung_function.dfev1_re_sds[1];
  (*ag).lung_function_LPT=(*ag).local_time;
  
#if OUTPUT_EX>1
  output_ex.cumul_non_COPD_time+=(*ag).local_time;
#endif
#if (OUTPUT_EX & OUTPUT_EX_COPD) > 0 
  output_ex.n_inc_COPD_by_ctime_age[(int)floor((*ag).time_at_creation+(*ag).local_time)][(int)floor((*ag).age_at_creation+(*ag).local_time)]+=1;
#endif
}







//////////////////////////////////////////////////////////////////EVENT_EXACERBATIN////////////////////////////////////;
double event_exacerbation_tte(agent *ag)
{
  if((*ag).gold==0 || (*ag).exac_status>0) return(HUGE_VAL);
  
  double rate=exp((*ag).ln_exac_rate_intercept
                +input.exacerbation.ln_rate_betas[0]
                +input.exacerbation.ln_rate_betas[1]*(*ag).sex
                +input.exacerbation.ln_rate_betas[2]*((*ag).age_at_creation+(*ag).local_time)
                +input.exacerbation.ln_rate_betas[3]*(*ag).fev1
                +input.exacerbation.ln_rate_betas[4]*(*ag).smoking_status
                );
  
#ifdef PROJECT_ROC16
  (*ag).ROC16_exac_rate=rate;
  if((*ag).ROC16_treatment_status==1)
    rate=rate*input.project_specific.ROC16_treatment_RR;
#endif
  
  if((*ag).medication_status>0)
  {
     for(int i=0;i<N_MED_CLASS;i++)
      if(((*ag).medication_status >> i) & 1)
        rate=rate*exp(input.medication.ln_rr_exac_by_class[i]);
  }
  
  double tte;
    
  if(rate==0) tte=HUGE_VAL; else tte=rand_exp()/rate;
  
  return(tte);
}





void event_exacerbation_process(agent *ag)
{
  double temp=(*ag).logit_exac_severity_intercept
    +input.exacerbation.logit_severity_betas[2]*(*ag).sex
    +input.exacerbation.logit_severity_betas[3]*((*ag).age_at_creation+(*ag).local_time)
    +input.exacerbation.logit_severity_betas[4]*(*ag).fev1
    +input.exacerbation.logit_severity_betas[5]*(*ag).smoking_status;
    
  double l1,l2;
  l1=temp+input.exacerbation.logit_severity_betas[0];
  l2=temp+input.exacerbation.logit_severity_betas[1];
  double p0,p1,p2;
  p1=exp(l1)/(1+exp(l1)+exp(l2));
  p2=exp(l2)/(1+exp(l1)+exp(l2));
  p0=1-p1-p2;
  
  double r=rand_unif();
  
  if(r<p0) (*ag).exac_status=1; else if(r<p0+p1) (*ag).exac_status=2; else (*ag).exac_status=3;
  
  (*ag).cumul_exac[(*ag).exac_status-1]+=1;
  (*ag).exac_LPT=(*ag).local_time;
  
#if (OUTPUT_EX & OUTPUT_EX_MEDICATION) > 0
  if((*ag).medication_status>0)
  {
    for(int i=0;i<N_MED_CLASS;i++)
      if(((*ag).medication_status >> i) & 1)
      {
        output_ex.n_exac_by_medication_class[i][(*ag).exac_status-1]+=1;  
      }
  }
#endif
  
#if (OUTPUT_EX & OUTPUT_EX_EXACERBATION)>0
  output_ex.n_exac_by_ctime_age[(int)floor((*ag).time_at_creation+(*ag).local_time)][(int)floor((*ag).age_at_creation+(*ag).local_time)]+=1; 
  output_ex.n_exac_by_ctime_severity[(int)floor((*ag).time_at_creation+(*ag).local_time)][(*ag).exac_status-1]+=1; 
#endif
}







//////////////////////////////////////////////////////////////////EVENT_EXACERBATIN_END////////////////////////////////////;
double event_exacerbation_end_tte(agent *ag)
{
  if((*ag).exac_status==0) return(HUGE_VAL);
  
  double rate=input.exacerbation.exac_end_rate[(*ag).exac_status-1];
  
  double tte;
    
  if(rate==0) tte=HUGE_VAL; else tte=rand_exp()/rate;
  
  return(tte);
}





void event_exacerbation_end_process(agent *ag)
{
  (*ag).cumul_cost+=input.cost.exac_dcost[(*ag).exac_status-1]/(1+pow(input.global_parameters.discount_cost,(*ag).time_at_creation+(*ag).local_time));
  (*ag).cumul_qaly+=input.utility.exac_dutil[(*ag).exac_status-1]/(1+pow(input.global_parameters.discount_qaly,(*ag).time_at_creation+(*ag).local_time));
  (*ag).exac_status=0;
  
#ifdef PROJECT_ROC16
  //See if this is the first moderate or severe exac
  if((*ag).cumul_exac[1]+(*ag).cumul_exac[2]==1)
  {
    output.n_eligible++;
    //apply the biomarker(*ag).cumul_exac
    double biomarker=(*ag).ROC16_exac_rate*exp(rand_norm()*input.project_specific.ROC16_biomarker_noise_sd);
    (*ag).ROC16_biomarker=biomarker;
    output.total_cost+=input.project_specific.ROC16_biomarker_cost/pow(1+input.global_parameters.discount_cost,(*ag).local_time+(*ag).time_at_creation);
    if(biomarker>input.project_specific.ROC16_biomarker_threshold)
    {
      (*ag).ROC16_treatment_status=1;
      output.n_treated++;
    }
  }
#endif
}












//////////////////////////////////////////////////////////////////EVENT_EXACERBATIN_DEATH////////////////////////////////////;
double event_exacerbation_death_tte(agent *ag)
{
  if((*ag).exac_status==0) return(HUGE_VAL);
  
  double tte=HUGE_VAL;
  
  double p=input.exacerbation.p_death[(*ag).exac_status-1];
  
  if(rand_unif()<p)
  {
    tte=1/input.exacerbation.exac_end_rate[(*ag).exac_status-1];
    //All death occur at the end of expected time of exacerbation (for now);
    (*ag).local_time+=tte;
    return(0);
  }
  else
  {
    return(HUGE_VAL);
  }
  
}





void event_exacerbation_death_process(agent *ag)
{
  (*ag).alive=false;
  //Rprintf("Death by chocolate!\n");
}



////////////////////////////////////////////////////////////////////comorbidity events/////////////////////////////////////;
double event_mi_tte(agent *ag)
{
  double age=(*ag).local_time+(*ag).age_at_creation;
  double time=calendar_time+(*ag).local_time;
  //int age_cut=floor(age);
  
  double rate=exp(
    input.comorbidity.ln_h_mi_betas_by_sex[0][(*ag).sex]
    +input.comorbidity.ln_h_mi_betas_by_sex[1][(*ag).sex]*age
    +input.comorbidity.ln_h_mi_betas_by_sex[2][(*ag).sex]*age*age
    +input.comorbidity.ln_h_mi_betas_by_sex[3][(*ag).sex]*(*ag).pack_years
    +input.comorbidity.ln_h_mi_betas_by_sex[4][(*ag).sex]*(*ag).smoking_status
    +input.comorbidity.ln_h_mi_betas_by_sex[5][(*ag).sex]*time
    +input.comorbidity.ln_h_mi_betas_by_sex[6][(*ag).sex]*((*ag).weight/pow((*ag).height,2))
    +input.comorbidity.ln_h_mi_betas_by_sex[7][(*ag).sex]*(*ag).gold
    +input.comorbidity.ln_h_mi_betas_by_sex[8][(*ag).sex]*((*ag).n_mi>0)
    +input.comorbidity.ln_h_mi_betas_by_sex[9][(*ag).sex]*(*ag).n_mi
    );

  if(rate==0) return(HUGE_VAL);
  return(rand_exp()/rate);
}



void event_mi_process(agent *ag)
{  
  (*ag).n_mi++;
  (*ag).cumul_cost+=input.cost.mi_dcost;
  if(rand_unif()<input.comorbidity.p_mi_death)
  {
    (*ag).alive=0;
  }
#if (OUTPUT_EX & OUTPUT_EX_COMORBIDITY) > 0 
  output_ex.n_incident_mi++;
  output_ex.n_mi_by_age_sex[(int)floor((*ag).local_time+(*ag).age_at_creation)][(*ag).sex]++;
  output_ex.n_mi_by_ctime_sex[(int)floor((*ag).local_time+calendar_time)][(*ag).sex]++;
#endif
}





double event_stroke_tte(agent *ag)
{
  double age=(*ag).local_time+(*ag).age_at_creation;
  double time=calendar_time+(*ag).local_time;
  //int age_cut=floor(age);
  
  double rate=exp(
    input.comorbidity.ln_h_stroke_betas_by_sex[0][(*ag).sex]
    +input.comorbidity.ln_h_stroke_betas_by_sex[1][(*ag).sex]*age
    +input.comorbidity.ln_h_stroke_betas_by_sex[2][(*ag).sex]*age*age
    +input.comorbidity.ln_h_stroke_betas_by_sex[3][(*ag).sex]*(*ag).pack_years
    +input.comorbidity.ln_h_stroke_betas_by_sex[4][(*ag).sex]*(*ag).smoking_status
    +input.comorbidity.ln_h_stroke_betas_by_sex[5][(*ag).sex]*time
    +input.comorbidity.ln_h_stroke_betas_by_sex[6][(*ag).sex]*((*ag).weight/pow((*ag).height,2))
    +input.comorbidity.ln_h_stroke_betas_by_sex[7][(*ag).sex]*(*ag).gold
    +input.comorbidity.ln_h_stroke_betas_by_sex[8][(*ag).sex]*((*ag).n_mi>0)
    +input.comorbidity.ln_h_stroke_betas_by_sex[9][(*ag).sex]*(*ag).n_mi
    +input.comorbidity.ln_h_stroke_betas_by_sex[10][(*ag).sex]*((*ag).n_stroke>0)
    +input.comorbidity.ln_h_stroke_betas_by_sex[11][(*ag).sex]*(*ag).n_stroke
    );

  if(rate==0) return(HUGE_VAL);
  return(rand_exp()/rate);
}


void event_stroke_process(agent *ag)
{  
  (*ag).n_stroke++;
  (*ag).cumul_cost+=input.cost.stroke_dcost;
  if(rand_unif()<input.comorbidity.p_stroke_death)
  {
    (*ag).alive=0;
  }
#if (OUTPUT_EX & OUTPUT_EX_COMORBIDITY) > 0 
  output_ex.n_incident_stroke++;
  output_ex.n_stroke_by_age_sex[(int)floor((*ag).local_time+(*ag).age_at_creation)][(*ag).sex]++;
  output_ex.n_stroke_by_ctime_sex[(int)floor((*ag).local_time+calendar_time)][(*ag).sex]++;
#endif
}





double event_hf_tte(agent *ag)
{
  if((*ag).hf_status>0) return(HUGE_VAL);
  
  double age=(*ag).local_time+(*ag).age_at_creation;
  double time=calendar_time+(*ag).local_time;
  //int age_cut=floor(age);
  
  double rate=exp(
    input.comorbidity.ln_h_hf_betas_by_sex[0][(*ag).sex]
    +input.comorbidity.ln_h_hf_betas_by_sex[1][(*ag).sex]*age
    +input.comorbidity.ln_h_hf_betas_by_sex[2][(*ag).sex]*age*age
    +input.comorbidity.ln_h_hf_betas_by_sex[3][(*ag).sex]*(*ag).pack_years
    +input.comorbidity.ln_h_hf_betas_by_sex[4][(*ag).sex]*(*ag).smoking_status
    +input.comorbidity.ln_h_hf_betas_by_sex[5][(*ag).sex]*time
    +input.comorbidity.ln_h_hf_betas_by_sex[6][(*ag).sex]*((*ag).weight/pow((*ag).height,2))
    +input.comorbidity.ln_h_hf_betas_by_sex[7][(*ag).sex]*(*ag).gold
    +input.comorbidity.ln_h_hf_betas_by_sex[8][(*ag).sex]*((*ag).n_mi>0)
    +input.comorbidity.ln_h_hf_betas_by_sex[9][(*ag).sex]*(*ag).n_mi
    +input.comorbidity.ln_h_hf_betas_by_sex[10][(*ag).sex]*((*ag).n_stroke>0)
    +input.comorbidity.ln_h_hf_betas_by_sex[11][(*ag).sex]*(*ag).n_stroke
    );

  if(rate==0) return(HUGE_VAL);
  return(rand_exp()/rate);
}


void event_hf_process(agent *ag)
{  
  (*ag).hf_status=1;
#if (OUTPUT_EX & OUTPUT_EX_COMORBIDITY) > 0 
  output_ex.n_incident_hf++;
#endif
}

////////////////////////////////////////////////////////////////////EVENT_bgd/////////////////////////////////////;
double event_bgd_tte(agent *ag)
{
  double age=(*ag).local_time+(*ag).age_at_creation;
  double time=(*ag).time_at_creation+(*ag).local_time;
  int age_cut=floor(age);
  
  double _or=exp(
    input.agent.ln_h_bgd_betas[0]
    +input.agent.ln_h_bgd_betas[1]*time
    +input.agent.ln_h_bgd_betas[2]*time*time
    +input.agent.ln_h_bgd_betas[3]*age
    +input.agent.ln_h_bgd_betas[4]*((*ag).n_mi>0)
    +input.agent.ln_h_bgd_betas[5]*((*ag).n_mi)
    +input.agent.ln_h_bgd_betas[6]*((*ag).n_stroke>0)
    +input.agent.ln_h_bgd_betas[7]*((*ag).n_stroke)
    +input.agent.ln_h_bgd_betas[8]*((*ag).hf_status));
  
  double ttd=HUGE_VAL;
  double p=input.agent.p_bgd_by_sex[age_cut][(int)(*ag).sex];
  if(p==0) return(ttd);
  
  double odds=p/(1-p)*_or;
  p=odds/(1+odds);
  
  if(p==1) 
  {
    ttd=0;
  }
  else
  {
    double rate=-log(1-p);
    if(rate>0)  ttd=rand_exp()/rate; else ttd=HUGE_VAL;
    //if(rand_unif()<p) ttd=rand_unif();
  }
  return(ttd);
}





void event_bgd_process(agent *ag)
{
  (*ag).alive=false;
}















//////////////////////////////////////////////////////////////////EVENT_DOCTOR_VISIT////////////////////////////////////;
double event_doctor_visit_tte(agent *ag)
{
  return(HUGE_VALL); //Currently deisabled;
  double rate=input.outpatient.rate_doctor_visit;
  
  double tte=rand_exp()/rate;
  
  return(tte);
}





void event_doctor_visit_process(agent *ag)
{
  if(rand_unif()<input.outpatient.p_specialist)
  {
      (*ag).last_doctor_visit_type=1;
  }
  else
  {
      (*ag).last_doctor_visit_type=0;
  }
  
  output.total_doctor_visit[(*ag).last_doctor_visit_type]+=1;
  
  output.total_cost+=input.cost.doctor_visit_by_type[(*ag).last_doctor_visit_type]/pow(1+input.global_parameters.discount_cost,(*ag).local_time+calendar_time);
  
  (*ag).last_doctor_visit_time=(*ag).local_time;
}










//////////////////////////////////////////////////////////////////EVENT_MEDICATION_CHANGE////////////////////////////////////;
double event_medicaiton_change_tte(agent *ag)
{
  //if((*ag).medication_status==(*ag).recommended_)
  return(HUGE_VAL);
}





void event_medication_change_process(agent *ag)
{
  //medication_LPT(ag);
}









/////////////////////////////////////////////////////////////////////EVENT_FIXED/////////////////////////////////;
#define EVENT_FIXED_FREQ 1

double event_fixed_tte(agent *ag) 
{
    return(floor((*ag).local_time*EVENT_FIXED_FREQ)/EVENT_FIXED_FREQ+1/EVENT_FIXED_FREQ-(*ag).local_time);
}



agent *event_fixed_process(agent *ag)
{
  (*ag).weight+=input.agent.weight_0_betas[6];
  (*ag).weight_LPT=(*ag).local_time;
  
  lung_function_LPT(ag);
  smoking_LPT(ag);
  exacerbation_LPT(ag);
  payoffs_LPT(ag);
  #ifdef OUTPUT_EX
    update_output_ex(ag);
  #endif
  

  
   //COPD;
  double COPD_odds=exp(input.COPD.logit_p_COPD_betas_by_sex[0][(*ag).sex]
                      +input.COPD.logit_p_COPD_betas_by_sex[1][(*ag).sex]*(*ag).age_at_creation
                      +input.COPD.logit_p_COPD_betas_by_sex[2][(*ag).sex]*(*ag).age_at_creation*(*ag).age_at_creation
                      +input.COPD.logit_p_COPD_betas_by_sex[3][(*ag).sex]*(*ag).pack_years
                      +input.COPD.logit_p_COPD_betas_by_sex[4][(*ag).sex]*(*ag).smoking_status
                      +input.COPD.logit_p_COPD_betas_by_sex[5][(*ag).sex]*calendar_time)
                      //+input.COPD.logit_p_COPD_betas_by_sex[7]*(*ag).asthma
              ;            
  
  (*ag).p_COPD=COPD_odds/(1+COPD_odds);
  
#ifdef PROJECT_ROC16
  if((*ag).ROC16_treatment_status==1)
    output.total_cost+=input.project_specific.ROC16_treatment_cost/pow(1+input.global_parameters.discount_cost,(*ag).time_at_creation+(*ag).local_time);
#endif
  
  return(ag);
}








/////////////////////////////////////////////////////////event_birthday/////////////////////////////////////////////////

double event_birthday_tte(agent *ag)
{
  return(HUGE_VAL);
  double age=(*ag).age_at_creation+(*ag).local_time;
  double delta=1-(age-floor(age));
  if(delta==0) delta=1;
  //Rprintf("%f,%f\n",delta,(*ag).local_time+(*ag).age_at_creation);
  return(delta);
}


agent *event_birthday_process(agent *ag)
{
  //Rprintf("%f\n",(*ag).local_time+(*ag).age_at_creation);
  return(ag);
}






/////////////////////////////////////////////////////////////////////////MODEL///////////////////////////////////////////

// [[Rcpp::export]]
int Callocate_resources()
{
  if(runif_buffer==NULL)
    runif_buffer=(double *)malloc(settings.runif_buffer_size*sizeof(double));
  else
    realloc(runif_buffer,settings.runif_buffer_size*sizeof(double));
  if(runif_buffer==NULL) return(ERR_MEMORY_ALLOCATION_FAILED); 
  runif_buffer_pointer=settings.runif_buffer_size; //invoikes fill next time;
  
  if(rnorm_buffer==NULL)
    rnorm_buffer=(double *)malloc(settings.rnorm_buffer_size*sizeof(double));
  else
   realloc(rnorm_buffer,settings.rnorm_buffer_size*sizeof(double));
  if(rnorm_buffer==NULL) return(ERR_MEMORY_ALLOCATION_FAILED);
  rnorm_buffer_pointer=settings.rnorm_buffer_size;
  
  if(rexp_buffer==NULL)
    rexp_buffer=(double *)malloc(settings.rexp_buffer_size*sizeof(double));
  else
    realloc(rexp_buffer,settings.rexp_buffer_size*sizeof(double));
  if(rexp_buffer==NULL) return(ERR_MEMORY_ALLOCATION_FAILED);
  rexp_buffer_pointer=settings.rexp_buffer_size;
  
  if(agent_stack==NULL)
    agent_stack=(agent *)malloc(settings.agent_stack_size*sizeof(agent));
  else
    realloc(agent_stack,settings.agent_stack_size*sizeof(agent));
  if(agent_stack==NULL) return(ERR_MEMORY_ALLOCATION_FAILED);
  agent_stack_pointer=0;
  
  if(event_stack==NULL)
    event_stack=(agent *)malloc(settings.event_stack_size*sizeof(agent));
  else
    realloc(event_stack,settings.event_stack_size*sizeof(agent));
  if(event_stack==NULL) return(ERR_MEMORY_ALLOCATION_FAILED);

  return(0);
}


// [[Rcpp::export]]
List Cget_pointers()
{
  return(Rcpp::List::create(
      //Rcpp::Named("runif_buffer_address")=reinterpret_cast<long &>(runif_buffer),
      //Rcpp::Named("rnorm_buffer_address")=reinterpret_cast<long &>(rnorm_buffer),
      //Rcpp::Named("rexp_buffer_address")=reinterpret_cast<long &>(rexp_buffer),
      //Rcpp::Named("agent_stack")=reinterpret_cast<long &>(agent_stack),
      //Rcpp::Named("event_stack")=reinterpret_cast<long &>(event_stack)
    )
  );
}



// [[Rcpp::export]]
int Cdeallocate_resources()
{
  try
  {
    if(runif_buffer!=NULL) {free(runif_buffer); runif_buffer=NULL;}
    if(rnorm_buffer!=NULL) {free(rnorm_buffer); rnorm_buffer=NULL;}
    if(rexp_buffer!=NULL) {free(rexp_buffer); rexp_buffer=NULL;}
    if(agent_stack!=NULL) {free(agent_stack); agent_stack=NULL;}
    if(event_stack!=NULL) {free(event_stack); event_stack=NULL;}
  }catch(const std::exception& e){};
  return(0);
}




// [[Rcpp::export]]
int Cdeallocate_resources2()
{
  try
  {
    delete[] runif_buffer;
    delete[] rnorm_buffer;
    delete[] rexp_buffer;
    delete[] agent_stack;
    delete[] event_stack;
  }catch(const std::exception& e){};
  return(0);
}





int Callocate_resources2()
{
   //runif_buffer=(double *)malloc(runif_buffer_size*sizeof(double));
  runif_buffer=new double[settings.runif_buffer_size];
  if(runif_buffer==NULL) return(ERR_MEMORY_ALLOCATION_FAILED); 
  runif_buffer_pointer=settings.runif_buffer_size; //invoikes fill next time;
  
  rnorm_buffer=new double[settings.rnorm_buffer_size];
  if(rnorm_buffer==NULL) return(ERR_MEMORY_ALLOCATION_FAILED);
  rnorm_buffer_pointer=settings.rnorm_buffer_size;
  
  rexp_buffer=new double[settings.rexp_buffer_size];
  if(rexp_buffer==NULL) return(ERR_MEMORY_ALLOCATION_FAILED);
  rexp_buffer_pointer=settings.rexp_buffer_size;
  
  agent_stack=new agent[settings.agent_stack_size];
  if(agent_stack==NULL) return(ERR_MEMORY_ALLOCATION_FAILED);
  agent_stack_pointer=0;
  
  event_stack=new agent[settings.event_stack_size];
  if(event_stack==NULL) return(ERR_MEMORY_ALLOCATION_FAILED);

  return(0);
}


// [[Rcpp::export]]
int Cinit_session() //Does not deal with memory allocation only resets counters etc;
{
  event_stack_pointer=0;
  
  reset_output();
  reset_output_ex();
  reset_runtime_stats(); runtime_stats.agent_size=sizeof(agent);
  
  calendar_time=0;
  last_id=0;
  
  return(0);
}




// [[Rcpp::export]]
int Cmodel(int max_n_agents)
{
  if(max_n_agents<1) return(0);
  
  agent *ag;
  
  while(calendar_time<input.global_parameters.time_horizon && max_n_agents>0)
  //for(int i=0;i<n_agents;i++)
  {
    max_n_agents--;
    //calendar_time=0; NO! calendar_time is set to zero at init_session. Cmodel should be resumable;
        
    switch(settings.agent_creation_mode)
    {
      case agent_creation_mode_one:
        ag=create_agent(&smith,last_id);
        break;
        
      case agent_creation_mode_all:
        ag=create_agent(&agent_stack[last_id],last_id);
        break;
        
      case agent_creation_mode_pre:
        ag=&agent_stack[last_id];
        break;
        
      default:
        return(-1);
    }
    
    (*ag).tte=0;
    event_start_process(ag);
    (*ag).event=event_start;
    if(settings.record_mode==record_mode_event || settings.record_mode==record_mode_agent || settings.record_mode==record_mode_some_event)
    {
      int _res=push_event(ag);
      if(_res<0) return(_res);
    }

          
    while(calendar_time+(*ag).local_time<input.global_parameters.time_horizon && (*ag).alive && (*ag).age_at_creation+(*ag).local_time<MAX_AGE)
    {
      double tte=input.global_parameters.time_horizon-calendar_time-(*ag).local_time;;
      int winner=-1;
      double temp;
      
      temp=event_fixed_tte(ag);
      if(temp<tte)
      {
        tte=temp;
        winner=event_fixed;
      }
      
      temp=event_birthday_tte(ag);
      if(temp<tte)
      {
        tte=temp;
        winner=event_birthday;
      }
      
      temp=event_smoking_change_tte(ag);
      if(temp<tte)
      {
        tte=temp;
        winner=event_smoking_change;
      }
      
      temp=event_COPD_tte(ag);
      if(temp<tte)
      {
        tte=temp;
        winner=event_COPD;
      }
      
      temp=event_exacerbation_tte(ag);
      if(temp<tte)
      {
        tte=temp;
        winner=event_exacerbation;
      }
      
      temp=event_exacerbation_end_tte(ag);
      if(temp<tte)
      {
        tte=temp;
        winner=event_exacerbation_end;
      }
      
      temp=event_exacerbation_death_tte(ag);
      if(temp<tte)
      {
        tte=temp;
        winner=event_exacerbation_death;
      }
      
      temp=event_doctor_visit_tte(ag);
      if(temp<tte)
      {
        tte=temp;
        winner=event_doctor_visit;
      }
      
      temp=event_medicaiton_change_tte(ag);
      if(temp<tte)
      {
        tte=temp;
        winner=event_medication_change;
      }
      
      /*temp=event_mi_tte(ag);
      if(temp<tte)
      {
        tte=temp;
        winner=event_mi;
      }
      
      temp=event_stroke_tte(ag);
      if(temp<tte)
      {
        tte=temp;
        winner=event_stroke;
      }
      
      temp=event_hf_tte(ag);
      if(temp<tte)
      {
        tte=temp;
        winner=event_hf;
      }*/
      
      temp=event_bgd_tte(ag);
      if(temp<tte)
      {
        tte=temp;
        winner=event_bgd;
      }
                      
              
      if(calendar_time+(*ag).local_time+tte<input.global_parameters.time_horizon)
      {
        (*ag).tte=tte;
        (*ag).local_time=(*ag).local_time+tte;
        
        if(settings.update_continuous_outcomes_mode==1)
        {
          lung_function_LPT(ag);
          smoking_LPT(ag);
          exacerbation_LPT(ag);
          payoffs_LPT(ag);
          medication_LPT(ag);
        }      
        
        switch(winner)
        {
          case event_fixed:
            event_fixed_process(ag);
            (*ag).event=event_fixed;
            break;
          case event_birthday:
            event_birthday_process(ag);
            (*ag).event=event_birthday;
            break;
          case event_smoking_change:
            event_smoking_change_process(ag);
            (*ag).event=event_smoking_change;
            break;
          case event_COPD:
            event_COPD_process(ag);
            (*ag).event=event_COPD;
            break;
          case event_exacerbation:
            event_exacerbation_process(ag);
            (*ag).event=event_exacerbation;
            break;
          case event_exacerbation_end:
            event_exacerbation_end_process(ag);
            (*ag).event=event_exacerbation_end;
            break;
          case event_exacerbation_death:
            event_exacerbation_death_process(ag);
            (*ag).event=event_exacerbation_death;
            break;
          case event_doctor_visit:
            event_doctor_visit_process(ag);
            (*ag).event=event_doctor_visit;
            break;
          case event_medication_change:
            event_medication_change_process(ag);
            (*ag).event=event_medication_change;
            break;
          /*case event_mi:
            event_mi_process(ag);
            (*ag).event=event_mi;
            break;
          case event_stroke:
            event_stroke_process(ag);
            (*ag).event=event_stroke;
            break;
          case event_hf:
            event_hf_process(ag);
            (*ag).event=event_hf;
            break;*/
          case event_bgd:
            event_bgd_process(ag);
            (*ag).event=event_bgd;
            break;
        }
        if(settings.record_mode==record_mode_event || settings.record_mode==record_mode_some_event)
        {
          int _res=push_event(ag);
          if(_res<0) return(_res);
        }
      }
      else
      {//past TH, set the local time to TH as the next step will be agent end;
        (*ag).tte=input.global_parameters.time_horizon-calendar_time-(*ag).local_time;
        (*ag).local_time=(*ag).local_time+(*ag).tte;
      }
    }//while (within agent)
    
    event_end_process(ag);
    (*ag).event=event_end;
    if(settings.record_mode==record_mode_event || settings.record_mode==record_mode_agent || settings.record_mode==record_mode_some_event)
    {
      int _res=push_event(ag);
      if(_res<0) return(_res);
    }
    
    if(output.n_agents>settings.n_base_agents)  //now we are done with prevalent cases and are creating incident cases;
    {
      double incidence=exp(input.agent.l_inc_betas[0]+input.agent.l_inc_betas[1]*calendar_time+input.agent.l_inc_betas[2]*calendar_time*calendar_time);
      if(incidence<0.000000000000001) calendar_time=input.global_parameters.time_horizon; else  calendar_time=calendar_time+1/incidence/settings.n_base_agents;
    }
    last_id++;
  }//Outer while (between-agent)
  return(0);
}










