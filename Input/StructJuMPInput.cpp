#include "StructJuMPInput.h"

#include "../PIPS-NLP/par_macro.h"


#include <string>
#include <iostream>
#include <sstream>

StructJuMPInput::StructJuMPInput(PipsNlpProblemStruct* p)
{
	PAR_DEBUG("enter constructor StructJuMPInput - "<<p);
	this->prob = p;
	useInputDate = 1;
	datarootname = "StructJuMP";
	PAR_DEBUG("exit constructor StructJuMPInput - ");
}
StructJuMPInput::~StructJuMPInput() {

}

int StructJuMPInput::nScenarios() {
	return prob->nnodes;
}

void StructJuMPInput::get_prob_info(int nodeid)
{
	PAR_DEBUG("get_prob_info - prob_info - "<<nodeid);
	int nv = 0;
	int mc = 0;

	CallBackData data={prob->userdata,nodeid,nodeid};
//	PAR_DEBUG("data -"<<&data);
	PAR_DEBUG("get_prob_info callback data ptr - "<<&data);
	PAR_DEBUG("get_prob_info userdata ptr - "<<prob->userdata);
	prob->prob_info(&nv,NULL,NULL,&mc,NULL,NULL,&data);
	nvar_map[nodeid] = nv;
	ncon_map[nodeid] = mc;

	std::vector<double> collb(nv);
	std::vector<double> colub(nv);
	std::vector<double> rowlb(mc);
	std::vector<double> rowub(mc);
	prob->prob_info(&nv, &collb[0], &colub[0], &mc, &rowlb[0], &rowub[0], &data);

	collb_map[nodeid] = collb;
	colub_map[nodeid] = colub;
	rowlb_map[nodeid] = rowlb;
	rowub_map[nodeid] = rowub;

	int e_mc=0;
	int i_mc=0;
	bool e=true; //equality constraint must be at front of list
	for(int i=0;i<rowlb.size();i++)
	{
		if(rowlb[i] == rowub[i]){
			e_mc++;
			assert(e); //equality constraint must be at front of list
		}
		else{
			e = false;
			assert(rowlb[i]<rowub[i]);
			i_mc++;
		}
	}
	assert(i_mc+e_mc == mc);
	e_ncon_map[nodeid] = e_mc;
	i_ncon_map[nodeid] = i_mc;
	PAR_DEBUG("get_prob_info - insert - "<<nodeid);
}

int StructJuMPInput::nFirstStageVars()
{
	PAR_DEBUG("nFirstStageVars");
	std::map<int,int>::iterator it = nvar_map.find(0);
	if(it!=nvar_map.end())
		return  it->second;
	get_prob_info(0);
	return nvar_map[0];
}
int StructJuMPInput::nFirstStageCons(){
	PAR_DEBUG("nFirstStageCons");
	std::map<int,int>::iterator it = ncon_map.find(0);
	if(it!=ncon_map.end())
		return  it->second;
	get_prob_info(0);
	return ncon_map[0];
}
int StructJuMPInput::nSecondStageVars(int scen){
	int nodeid = scen + 1;
	PAR_DEBUG("nSecondStageVars");
	std::map<int,int>::iterator it = nvar_map.find(nodeid);
	if(it!=nvar_map.end())
		return  it->second;
	get_prob_info(nodeid);
	return nvar_map[nodeid];
}
int StructJuMPInput::nSecondStageCons(int scen){
	int nodeid = scen + 1;
	PAR_DEBUG("nSecondStageCons");
	std::map<int,int>::iterator it = ncon_map.find(nodeid);
	if(it!=ncon_map.end())
		return  it->second;
	get_prob_info(nodeid);
	return ncon_map[nodeid];
}

std::vector<double> StructJuMPInput::getFirstStageColLB(){
	std::map<int, std::vector<double> >::iterator it = collb_map.find(0);
	if(it!=collb_map.end())
		return  it->second;
	get_prob_info(0);
	return collb_map[0];
}
std::vector<double> StructJuMPInput::getFirstStageColUB(){
	std::map<int, std::vector<double> >::iterator it = colub_map.find(0);
	if(it!=colub_map.end())
		return  it->second;
	get_prob_info(0);
	return colub_map[0];
}

std::vector<double> StructJuMPInput::getFirstStageObj(){
	PAR_DEBUG("getFirstStageObj - 0");
	assert(nvar_map.find(0)!=nvar_map.end());
	int nvar = nvar_map[0];
	double x0[nvar];
	std::vector<double> grad(nvar);
	CallBackData data = {prob->userdata,0,0};
	prob->eval_grad_f(x0,x0,&grad[0],&data);
	PAR_DEBUG("end getFirstStageObj - 0");
	return grad;
}
std::vector<std::string> StructJuMPInput::getFirstStageColNames(){
	assert(nvar_map.find(0)!=nvar_map.end());
	int nvar = nvar_map[0];
	std::vector<std::string> cnames(nvar);
	for(int i=0;i<nvar;i++)
	{
		std::ostringstream oss;
		oss<<"x"<<i;
		cnames[i] = oss.str();
	}
	return cnames;
}
std::vector<double> StructJuMPInput::getFirstStageRowLB(){
	std::map<int, std::vector<double> >::iterator it = rowlb_map.find(0);
	if(it!=rowlb_map.end())
		return  it->second;
	get_prob_info(0);
	return rowlb_map[0];
}
std::vector<double> StructJuMPInput::getFirstStageRowUB(){
	std::map<int, std::vector<double> >::iterator it = rowub_map.find(0);
	if(it!=rowub_map.end())
		return  it->second;
	get_prob_info(0);
	return rowub_map[0];
}
std::vector<std::string> StructJuMPInput::getFirstStageRowNames(){
	assert(ncon_map.find(0)!=ncon_map.end());
	int ncon = ncon_map[0];
	std::vector<std::string> cnames(ncon);
	for(int i=0;i<ncon;i++)
	{
		std::ostringstream oss;
		oss<<"c"<<i;
		cnames[i] = oss.str();
	}
	return cnames;
}
bool StructJuMPInput::isFirstStageColInteger(int col){
	return false;
}

std::vector<double> StructJuMPInput::getSecondStageColLB(int scen){
	int nodeid = scen + 1;
	std::map<int, std::vector<double> >::iterator it = collb_map.find(nodeid);
	if(it!=collb_map.end())
		return  it->second;
	get_prob_info(nodeid);
	return collb_map[nodeid];
}
std::vector<double> StructJuMPInput::getSecondStageColUB(int scen){
	int nodeid = scen + 1;
	std::map<int, std::vector<double> >::iterator it = colub_map.find(nodeid);
	if(it!=colub_map.end())
		return it->second;
	get_prob_info(nodeid);
	return colub_map[nodeid];
}
// objective vector, already multiplied by probability
std::vector<double> StructJuMPInput::getSecondStageObj(int scen){
	PAR_DEBUG("getSecondStageObj -  nodeid "<<scen+1);
	int nodeid = scen + 1;
	assert(nvar_map.find(nodeid)!=nvar_map.end());
	assert(nvar_map.find(0)!=nvar_map.end());
	int n0 = nvar_map[0];
	int n1 = nvar_map[nodeid];
	double x0[n0];
	double x1[n1];
	std::vector<double> grad(n1);
	CallBackData data = {prob->userdata,nodeid,nodeid};
	prob->eval_grad_f(x0,x1,&grad[0],&data);
	PAR_DEBUG("end getSecondStageObj - nodeid"<<scen+1);
	return grad;
}
std::vector<std::string> StructJuMPInput::getSecondStageColNames(int scen){
	int nodeid = scen + 1;
	assert(nvar_map.find(nodeid)!=nvar_map.end());
	assert(nvar_map.find(0)!=nvar_map.end());
	int i0 = nvar_map[0];
	int nvar = nvar_map[nodeid];
	std::vector<std::string> cnames(nvar);
	for(int i=0;i<nvar;i++)
	{
		std::ostringstream oss;
		oss<<"x"<<(i+i0);
		cnames[i] = oss.str();
	}
	return cnames;
}
std::vector<double> StructJuMPInput::getSecondStageRowUB(int scen){
	int nodeid = scen + 1;
	std::map<int, std::vector<double> >::iterator it = rowub_map.find(nodeid);
	if(it!=rowub_map.end())
		return  it->second;
	get_prob_info(nodeid);
	return rowub_map[nodeid];
}
std::vector<double> StructJuMPInput::getSecondStageRowLB(int scen){
	int nodeid = scen + 1;
	std::map<int, std::vector<double> >::iterator it = rowlb_map.find(nodeid);
	if(it!=rowlb_map.end())
		return  it->second;
	get_prob_info(nodeid);
	return rowlb_map[nodeid];
}
std::vector<std::string> StructJuMPInput::getSecondStageRowNames(int scen){
	int nodeid = scen + 1;
	assert(ncon_map.find(0)!=ncon_map.end());
	assert(ncon_map.find(nodeid)!=ncon_map.end());
	int ncon = ncon_map[nodeid];
	int i0 = ncon_map[0];
	std::vector<std::string> cnames(ncon);
	for(int i=0;i<ncon;i++)
	{
		std::ostringstream oss;
		oss<<"c"<<(i+i0);
		cnames[i] = oss.str();
	}
	return cnames;
}
double StructJuMPInput::scenarioProbability(int scen){
	return 1.0/scen;
}
bool StructJuMPInput::isSecondStageColInteger(int scen, int col){
	return false;
}

// returns the column-oriented first-stage constraint matrix (A matrix)
CoinPackedMatrix StructJuMPInput::getFirstStageConstraints(){
	PAR_DEBUG("getFirstStageConstraints - Amat -  ");
	int nvar = nvar_map[0];
	CallBackData cbd = {prob->userdata,0,0};
	std::vector<double> x0(nvar,1.0);
	int e_nz, i_nz;
	prob->eval_jac_g(&x0[0],&x0[0],
			&e_nz,NULL,NULL,NULL,
			&i_nz,NULL,NULL,NULL,&cbd);

	std::vector<int> e_rowidx(e_nz);
	std::vector<int> e_colptr(nvar+1,0);
	std::vector<double> e_elts(e_nz);

	std::vector<int> i_rowidx(i_nz);
	std::vector<int> i_colptr(nvar+1,0);
	std::vector<double> i_elts(i_nz);

	prob->eval_jac_g(&x0[0],&x0[0],
				&e_nz,&e_elts[0],&e_rowidx[0],&e_colptr[0],
				&i_nz,&i_elts[0],&i_rowidx[0],&i_colptr[0],&cbd);


	int e_ncon = e_ncon_map[0];
	int i_ncon = i_ncon_map[0];

	CoinPackedMatrix e_amat;
	amat.copyOf(true,e_ncon,nvar,0,&e_elts[0],&e_rowidx[0],&e_colptr[0],0);
	CoinPackedMatrix i_amat;
	i_amat.copyOf(true,i_ncon,nvar,0,&i_elts[0],&i_rowidx[0],&i_colptr[0],0);

	amat.bottomAppendPackedMatrix(i_amat);

	assert(amat.getNumCols()==nvar);
	assert(amat.getNumRows()==ncon_map[0]);
	PAR_DEBUG("getFirstStageConstraints "<<amat.getNumRows()<<" x "<<amat.getNumCols());
	return amat;
}
// returns the column-oriented second-stage constraint matrix (W matrix)
CoinPackedMatrix StructJuMPInput::getSecondStageConstraints(int scen){
	int nodeid = scen + 1;
	PAR_DEBUG("getSecondStageConstraints - Wmat -  "<<(scen+1));
	//Bmat + Dmat  = Wmat

	std::map<int,CoinPackedMatrix>::iterator it = wmat_map.find(nodeid);
	if(it!=wmat_map.end()){
		PAR_DEBUG("return (quick) getSecondStageConstraints - Wmat -  "<<it->second.getNumRows()<<" x "<<it->second.getNumCols());
		return it->second;
	}

	int nvar = nvar_map[nodeid];

	CallBackData cbd = {prob->userdata,nodeid,nodeid};
	std::vector<double> x0(nvar_map[0],1.0);
	std::vector<double> x1(nvar,1.0);

	int e_nz, i_nz;
	prob->eval_jac_g(&x0[0],&x1[0],
			&e_nz,NULL,NULL,NULL,
			&i_nz,NULL,NULL,NULL,&cbd);

	std::vector<int> e_rowidx(e_nz);
	std::vector<int> e_colptr(nvar+1,0);
	std::vector<double> e_elts(e_nz);

	std::vector<int> i_rowidx(i_nz);
	std::vector<int> i_colptr(nvar+1,0);
	std::vector<double> i_elts(i_nz);

	prob->eval_jac_g(&x0[0],&x1[0],
			&e_nz,&e_elts[0],&e_rowidx[0],&e_colptr[0],
			&i_nz,&i_elts[0],&i_rowidx[0],&i_colptr[0],&cbd);

	int e_ncon = e_ncon_map[nodeid];
	int i_ncon = i_ncon_map[nodeid];

	CoinPackedMatrix wmat;
	CoinPackedMatrix i_wmat;
	wmat.copyOf(true,e_ncon,nvar,0,&e_elts[0],&e_rowidx[0],&e_colptr[0],0);
	i_wmat.copyOf(true,i_ncon,nvar,0,&i_elts[0],&i_rowidx[0],&i_colptr[0],0);

	wmat.bottomAppendPackedMatrix(i_wmat);

	wmat_map[scen] = wmat;
	assert(wmat.getNumCols()==nvar);
	assert(wmat.getNumRows()==ncon_map[nodeid]);
	PAR_DEBUG("return getSecondStageConstraints - Wmat -  "<<wmat.getNumRows()<<" x "<<nvar);
	return wmat;
}
// returns the column-oriented matrix linking the first-stage to the second (T matrix)
CoinPackedMatrix StructJuMPInput::getLinkingConstraints(int scen){
	PAR_DEBUG("getLinkingConstraints - Tmat -  "<<(scen+1));
	int nodeid = scen + 1;

	//Amat + Cmat  = Tmat

	std::map<int,CoinPackedMatrix>::iterator it = tmat_map.find(nodeid);
	if(it!=tmat_map.end()) {
		PAR_DEBUG("return (quick) getLinkingConstraints - Tmat - "<<it->second.getNumRows()<<" x "<<it->second.getNumCols());
		return it->second;
	}

	int nvar = nvar_map[0];
	CallBackData cbd = {prob->userdata,nodeid,0};
	std::vector<double> x0(nvar_map[0],1.0);
	std::vector<double> x1(nvar_map[nodeid],1.0);

	int e_nz, i_nz;
	prob->eval_jac_g(&x0[0],&x1[0],
			&e_nz,NULL,NULL,NULL,
			&i_nz,NULL,NULL,NULL,&cbd);

	std::vector<int> e_rowidx(e_nz);
	std::vector<int> e_colptr(nvar+1,0);
	std::vector<double> e_elts(e_nz);

	std::vector<int> i_rowidx(i_nz);
	std::vector<int> i_colptr(nvar+1,0);
	std::vector<double> i_elts(i_nz);


	prob->eval_jac_g(&x0[0],&x1[0],
			&e_nz,&e_elts[0],&e_rowidx[0],&e_colptr[0],
			&i_nz,&i_elts[0],&i_rowidx[0],&i_colptr[0],&cbd);

	int e_ncon = e_ncon_map[nodeid];
	int i_ncon = i_ncon_map[nodeid];

	CoinPackedMatrix tmat;
	CoinPackedMatrix i_tmat;
	tmat.copyOf(true,e_ncon,nvar,0,&e_elts[0],&e_rowidx[0],&e_colptr[0],0);
	i_tmat.copyOf(true,i_ncon,nvar,0,&i_elts[0],&i_rowidx[0],&i_colptr[0],0);

	tmat.bottomAppendPackedMatrix(i_tmat);

	tmat_map[nodeid] = tmat;
	assert(tmat.getNumCols()==nvar);
	assert(tmat.getNumRows()==ncon_map[nodeid]);
	PAR_DEBUG("return getLinkingConstraints - Tmat - "<<tmat.getNumRows()<<" x "<<nvar);
	return tmat;
}

CoinPackedMatrix StructJuMPInput::getFirstStageHessian(){

	return qamat;
}
// Q_i
CoinPackedMatrix StructJuMPInput::getSecondStageHessian(int scen){
	int nodeid = scen + 1;

	return qwmat_map[nodeid];
}

// column-oriented, \hat Q_i
// Note: this has the second-stage variables on the rows and first-stage on the columns
CoinPackedMatrix StructJuMPInput::getSecondStageCrossHessian(int scen){
	int nodeid = scen + 1;

	return qtmat_map[nodeid];
}


// some problem characteristics that could be helpful to know

// all scenarios have the same number of variables and constraints
bool StructJuMPInput::scenarioDimensionsEqual(){
	return false;
}
// constraint (and hessian) matrices are identical for each scenario,
// column and row bounds and objective are allowed to vary
bool StructJuMPInput::onlyBoundsVary(){
	return false;
}
// all scenarios equally likely
bool StructJuMPInput::allProbabilitiesEqual(){
	return true;
}
// all second-stage variables continuous
bool StructJuMPInput::continuousRecourse(){
	return true;
}
