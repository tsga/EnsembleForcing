﻿//**********************************************************************************************

//
//********************************************************************************************** 

#include "ens_land.h"

//https://www.geeksforgeeks.org/program-distance-two-points-earth/

// C++ program to calculate Distance Between Two Points on Earth

// Utility function for
// converting degrees to radians
double toRadians(const double degree)
{
	// cmath library in C++
	// defines the constant
	// M_PI as the value of
	// pi accurate to 1e-30
	const long double My_PI = 3.1415926535897932384626433832795;
	long double one_deg = (My_PI) / 180.0;
	return (one_deg * degree);
}
double distance(double lat1, double long1, double lat2, double long2)
{
	// Convert the latitudes
	// and longitudes
	// from degree to radians.
	lat1 = toRadians(lat1);
	long1 = toRadians(long1);
	lat2 = toRadians(lat2);
	long2 = toRadians(long2);

	// Haversine Formula
	long double dlong = long2 - long1;
	long double dlat = lat2 - lat1;

	long double ans = pow(sin(dlat / 2), 2) +
		cos(lat1) * cos(lat2) *
		pow(sin(dlong / 2), 2);

	ans = 2 * asin(sqrt(ans));

	// Radius of Earth in
	// Kilometers, R = 6371
	// Use R = 3956 for miles
	long double R = 6371;

	// Calculate the result
	ans = ans * R;

	return ans;
}

modelGridCells::modelGridCells()   //,const char* daconFile)
{
	//daAssimlate = true;
	//updateDaArray = true;

	ns_statSize = 8;   //=inpDaControl.ns_statSize;
	mod_gridSize = 1;
	num_obs_points = 0;
	tot_gridSize = mod_gridSize + num_obs_points;    // mod_gridSize + numObsPoints;

	forc_stddev = 0.25;
	ens_size = 20;
	particle_size = 20;

	spat_corLength = 55000.0;  //m
	temp_decorrLength = 24.0;  //hr

	modelDT = 6.0;
	num_time_cycle = 24;  //number of time steps in (daily) cycle
	ini_time = 0;   //very initial time (no stored random pattern)

	corrFact1 = 1.0 - modelDT / temp_decorrLength;
	corrFact2 = sqrtf(1.0 - corrFact1 * corrFact1);    //1.0 - corrFact1;  // 

	/*std::vector<Eigen::Matrix<float, Dynamic, Dynamic, RowMajor>>
		multivarNormalDistSamplesForc(7,
			Eigen::Matrix<float, Dynamic, Dynamic, RowMajor>(tot_gridSize, ens_size));*/
	_multivarNormalDistSamplesForc.resize(n_vars,
		Eigen::Matrix<float, Dynamic, Dynamic, RowMajor>(tot_gridSize, ens_size));

	//startIndexDA = 0;

	//initDAMatrices(icellCoordinates);			//, daYcorrArr, daXcorrArr, uebDAsacrutpix7State);

	/*std::ofstream debugOutputFile;
	debugOutputFile.open("debugOutput.txt", std::ios::out);
	debugOutputFile.close();*/
}

modelGridCells::modelGridCells(int modGridCells, int numObsPoints, int ensSize, 
	int num_tim_cyc, int ini_tim, float modDT, float pert_facts[7])   //,const char* daconFile)
{
	//daAssimlate = true;
	//updateDaArray = true;

	ns_statSize = 8;   //=inpDaControl.ns_statSize;
	mod_gridSize = modGridCells;
	num_obs_points = numObsPoints;
	tot_gridSize = mod_gridSize + num_obs_points;    // mod_gridSize + numObsPoints;

	forc_stddev = 0.25;
	ens_size = ensSize;  // 1;
	particle_size = ens_size;

	spat_corLength = 55000.0;  //m
	temp_decorrLength = 24.0;  //hr

	modelDT = modDT;
	num_time_cycle = num_tim_cyc;
	ini_time = ini_tim;   //very initial time (no stored random pattern

	for (int i = 0; i < n_vars; i++) //n_vars must be <=8
	{
		std_dev[i] = pert_facts[i];
	}

	corrFact1 = 1.0 - modelDT / temp_decorrLength;
	corrFact2 = sqrtf(1.0 - corrFact1 * corrFact1);    //1.0 - corrFact1;  // 

	/*std::vector<Eigen::Matrix<float, Dynamic, Dynamic, RowMajor>>
		multivarNormalDistSamplesForc(7,
			Eigen::Matrix<float, Dynamic, Dynamic, RowMajor>(tot_gridSize, ens_size));*/
	_multivarNormalDistSamplesForc.resize(n_vars,
		Eigen::Matrix<float, Dynamic, Dynamic, RowMajor>(tot_gridSize, ens_size));
	//startIndexDA = 0;

	//initDAMatrices(icellCoordinates);			//, daYcorrArr, daXcorrArr, uebDAsacrutpix7State);

	/*std::ofstream debugOutputFile;
	debugOutputFile.open("debugOutput.txt", std::ios::out);
	debugOutputFile.close();*/
}

void modelGridCells::initDAMatrices(int myrank, 
	std::vector<float> lat_arry, std::vector<float> lon_arry,
	double &interm_time, bool use_seed, int seed_in) //int seed_in = 0
{

	if (int(lat_arry.size()) != tot_gridSize) {
		//if (myrank == 0) {
		std::cout << "proc " << myrank << " wrong lat vector size " << lat_arry.size() <<
			" gridcells vector size = " << tot_gridSize << " Exiting " << std::endl;
		//}
		MPI::COMM_WORLD.Abort(1);
	}

	double distance_ij;
	//, RowMajor
	Matrix<float, Dynamic, Dynamic> covarFd(tot_gridSize, tot_gridSize);
	for (int i = 0; i < tot_gridSize; i++) {
		for (int j = 0; j < tot_gridSize; j++) {
			//for (int j = 0; j < tot_gridSize; j++) {
			distance_ij =
				1000.0 * distance(lat_arry[i], lon_arry[i], lat_arry[j], lon_arry[j]);
			if (distance_ij > 20000000) {
				std::cout << " proc "<<myrank<<" possible error in distance caclculation " << std::endl;
				std::cout << distance_ij << std::endl; 
				std::cout << " Lat/lon " << lat_arry[i] << " " << lon_arry[i] << " " 
					<< lat_arry[j] << " " << lon_arry[j] << " " << std::endl;
				MPI::COMM_WORLD.Abort(1);
			}
			//covarFd(i, j) = exp(-1.0 * distance_ij * distance_ij
			//	/ (2 * spat_corLength));		//*daContArr.forcEnStdev * daContArr.forcEnStdev;
			covarFd(i, j) = (1.0 + distance_ij / spat_corLength)
				*exp(-1.0 * distance_ij / spat_corLength);
			/*ρ_jk = α(r_jk) β(z_jk) 		(A4)
              α(r_jk) = (1 + r_jk / L)* exp(-r_jk / L) (A5)
			  β(z_jk) = exp⁡(-⌊z_jk / h^ 2)      (A6)*/
		
			//covarFd(j, i) = covarFd(i, j);
			if (covarFd(i, j) <= 0.0000001) {
				/*std::cout << " proc " << myrank << "  error -ve/0 corr value " << std::endl;
				std::cout << covarFd(i, j) << " at i,j "<<i<<", "<<j<<std::endl;
				std::cout << " distance =" << distance_ij << std::endl;
				std::cout << " Lat/lon " << lat_arry[i] << " " << lon_arry[i] << " "
					<< lat_arry[j] << " " << lon_arry[j] << " " << std::endl;*/
				//MPI::COMM_WORLD.Abort(1);
				covarFd(i, j) = 0.0000001;
			}
		}
	}
	/*if (myrank == 0) {
		std::cout << "computed sptaial corr time " << MPI::Wtime() - interm_time << std::endl;
		interm_time = MPI::Wtime();
	}*/
	std::cout << "proc " << myrank << " computed sptaial corr time " << std::endl;

	//, RowMajor
	Matrix<float, Dynamic, Dynamic> corrFs(3, 3);   // correlation-between states
	corrFs << 1.0, -0.8, 0.5,
		-0.8, 1.0, -0.5,
		0.5, -0.5, 1.0;

	//corrFs_L.resize(3, 3);
	Eigen::LLT<Eigen::Matrix<float, Dynamic, Dynamic> > cholSolver(corrFs);
	if (cholSolver.info() == Eigen::Success)
		corrFs_L = cholSolver.matrixL();
	else
	{
		//8.27.18
		std::cout << "Failed computing the Cholesky decomposition. Using Eigen solver instead" << std::endl;
		SelfAdjointEigenSolver<Matrix<float, Dynamic, Dynamic> > corrFs_eigenSolver
			= SelfAdjointEigenSolver<Matrix<float, Dynamic, Dynamic> >(corrFs);
		corrFs_L = corrFs_eigenSolver.eigenvectors()*corrFs_eigenSolver.eigenvalues().cwiseMax(0).cwiseSqrt().asDiagonal();
	}
	if (myrank == 0) {
		std::cout << "L factorization of corr. matrix " << std::endl;
		std::cout << corrFs_L << std::endl;
		//std::cout << "computed factorization time " << MPI::Wtime() - interm_time << std::endl;
		//interm_time = MPI::Wtime();
	}
	std::cout << "proc " << myrank << " computed L fact " << std::endl;
	//Matrix<float, Dynamic, Dynamic, RowMajor> corrFs_r(3, 3);   // correlation-between states
	//corrFs_r << 1.0, -0.8, 0.5,
	//	-0.8, 1.0, -0.5,
	//	0.5, -0.5, 1.0;
	//Eigen::LLT<Eigen::Ref<Eigen::Matrix<float, Dynamic, Dynamic, RowMajor>> >
	//	i_cholSolver(corrFs_r);
	//
	//if (i_cholSolver.info() == Eigen::Success) {
	//	//Matrix<float, Dynamic, Dynamic> corrFs_L_r = i_cholSolver.matrixL();
	//	if (myrank == 0) {
	//		std::cout << "inplace L factorization of corr. matrix " << std::endl;
	//		std::cout << corrFs_r << std::endl;

	//		//corrFs_r.triangularView<StrictlyUpper>() = corrFs_r.adjoint();
	//		//.adjoint().triangularView<StrictlyUpper>();
	//		corrFs_r = corrFs_r.triangularView<Lower>();
	//		std::cout << "After lower triangular view" << std::endl;
	//		std::cout << corrFs_r << std::endl;
	//		//std::cout << "computed inplace factorization time " << MPI::Wtime() - interm_time << std::endl;
	//		//interm_time = MPI::Wtime();
	//	}
	//}
	//std::cout << "proc " << myrank << " computed inplace fact " << std::endl;
	uint64_t seedF;
	if (use_seed)  
		seedF = uint64_t(seed_in);
	else
		seedF = static_cast<uint64_t>(time(0));
	//9.1.16 set rand. generator
	//8.23.18 def seed: 
	std_norm_dist_Forc_Default.setSeed(seedF);	
	/*if (myrank == 0) {
		std::cout << "set seed " << std::endl;
		std::cout << " time " << MPI::Wtime() - interm_time << std::endl;
	}
	interm_time = MPI::Wtime();*/
	VectorXf meanF(tot_gridSize);
	meanF.setZero();
	std_norm_dist_Forc_Default.setMean(meanF);
	/*if (myrank == 0) {
		std::cout << "set mean time " << MPI::Wtime() - interm_time << std::endl;
		interm_time = MPI::Wtime();
	}*/
	std::cout << "proc " << myrank << " set mean " << std::endl;
	std_norm_dist_Forc_Default.setCovar(covarFd, true);
	/*if (myrank == 0) {
		std::cout << "set cov matrix time " << MPI::Wtime() - interm_time << std::endl;
		interm_time = MPI::Wtime();
	}*/
	std::cout << "proc " << myrank << " computed cov" << std::endl;
	//std::cout << " In Dafunc 2" << std::endl;

	return;
}

void modelGridCells::Generate_Samples(int myrank, double &interm_time, int istep) //= 0
{
	Eigen::Matrix<float, Dynamic, Dynamic>   //, RowMajor
		multivarNormalDistSamplesForc(tot_gridSize, ens_size);

	multivarNormalDistSamplesForc = std_norm_dist_Forc_Default.samples(ens_size);
	if (myrank == 0) {
		std::cout << "sample generate time " << MPI::Wtime() - interm_time << std::endl;
		interm_time = MPI::Wtime();
	}
	/*Eigen::Matrix<float, Dynamic, Dynamic, RowMajor>
		NormalDistSamplesForc(tot_gridSize, ens_size);*/
		//NormalDistSamplesForc = norm_dist_0Mean_Default.samples(ens_size);	

		/*Eigen::Matrix<float, Dynamic, Dynamic, RowMajor>
			NormalDistSamplesForc_mean1(tot_gridSize, ens_size);*/

			//NormalDistSamplesForc_mean1 = norm_dist_1Mean_Default.samples(ens_size);

	std::ofstream stdnormalSamplestxt("ens_err_all.txt");
	/*std::ofstream stdnormalSamplestxtV("ens_err_novarcor.txt");
	std::ofstream stdnormalSamplestxtV1("ens_err_novarcor_mean1.txt");*/
	stdnormalSamplestxt << multivarNormalDistSamplesForc << std::endl;
	//stdnormalSamplestxt.close();
	//stdnormalSamplestxtV << NormalDistSamplesForc << std::endl;
	//stdnormalSamplestxtV.close();

	//stdnormalSamplestxtV1 << NormalDistSamplesForc_mean1 << std::endl;
	//stdnormalSamplestxtV1.close();
	float fillVal = -999999.9999;
	if (istep != 0)
	{
		Eigen::Matrix<float, Dynamic, Dynamic>   //, RowMajor
			multivarNormalDistSamplesForc_in(tot_gridSize, ens_size);
		/*Eigen::Matrix<float, Dynamic, Dynamic, RowMajor>
			NormalDistSamplesForc_in(tot_gridSize, ens_size);
		Eigen::Matrix<float, Dynamic, Dynamic, RowMajor>
			NormalDistSamplesForc_mean1_in(tot_gridSize, ens_size);*/

		Read_random_samples(tot_gridSize, ens_size,
			multivarNormalDistSamplesForc_in,
			"ens_err_all.nc", "ens_err_all");
		/*Read_random_samples(tot_gridSize, ens_size,
			NormalDistSamplesForc_in,
			"ens_err_spatial.nc", "ens_err_spatial");
		Read_random_samples(8 * tot_gridSize, ens_size,
			NormalDistSamplesForc_mean1_in,
			"ens_err_spatial_mean1.nc", "ens_err_spatial_mean1");*/

		multivarNormalDistSamplesForc = corrFact1 * multivarNormalDistSamplesForc_in +
			corrFact2 * multivarNormalDistSamplesForc;
		/*NormalDistSamplesForc = corrFact1 * NormalDistSamplesForc_in +
										corrFact2 * NormalDistSamplesForc;
		NormalDistSamplesForc_mean1 = corrFact1 * NormalDistSamplesForc_mean1_in +
										corrFact2 * NormalDistSamplesForc_mean1;*/
	}
	std::cout << "temporal cor applied " << MPI::Wtime() - interm_time << std::endl;
	interm_time = MPI::Wtime();

	write_random_samples(tot_gridSize, ens_size,
		multivarNormalDistSamplesForc,
		"ens_err_all.nc", "ens_err_all", "locations_and_states", &fillVal);
	/*write_random_samples(tot_gridSize, ens_size,
		NormalDistSamplesForc,
		"ens_err_spatial.nc", "ens_err_spatial", "locations", &fillVal);
	write_random_samples(tot_gridSize, ens_size,
		NormalDistSamplesForc_mean1,
		"ens_err_spatial_mean1.nc", "ens_err_spatial_mean1", "locations", &fillVal);*/
		//if(istep == 2)	std::cout << " corFact = " << corrFact1 << " corFact2 = " << corrFact2 << std::endl;
	std::cout << "write samples time " << MPI::Wtime() - interm_time << std::endl;
	interm_time = MPI::Wtime();
	return;
}

//void modelGridCells::Create_Ensemble_Members(int myrank, int beg_indx,
//	std::string FileDir, std::string forc_inp_file,  double &interm_time)
//{
//
//	int ncid;   // = 0;   // , yx_dimid = 0, z_dimid = 0;   // , t_dimid = 0, 
//	int v_varid;   // = 0;     // , t_varid = 0;
//	// Error handling.  
//	int retncval = 0;
//
//	float*** forcVar = new float**[n_vars];
//	for (int is = 0; is < n_vars; is++) {
//		forcVar[is] = new float*[num_time_cycle];
//		for (int it = 0; it < num_time_cycle; it++)
//		{
//			forcVar[is][it] = new float[tot_gridSize];
//		}
//	}
//	float**** outArray = new float***[ens_size];
//	for (int ie = 0; ie < ens_size; ie++) {
//		outArray[ie] = new float**[n_vars];
//		for (int is = 0; is < n_vars; is++) {
//			outArray[ie][is] = new float*[num_time_cycle];
//			for (int it = 0; it < num_time_cycle; it++) {
//				outArray[ie][is][it] = new float[tot_gridSize];
//			}
//		}
//	}
//
//	size_t start[2], count[2];
//	start[0] = 0;
//	count[0] = num_time_cycle;
//	start[1] = beg_indx;
//	count[1] = tot_gridSize;
//
//	std::string FileName;
//	// read inputs
//	FileName = FileDir + "/" + forc_inp_file;     //"/ens01/" + 
//	//Open the netcdf file.  
//	if ((retncval = nc_open((const char*)FileName.c_str(), NC_NOWRITE, &ncid)))
//		ncERR(retncval, FileName);
//	//std::cout << "starting loop " << std::endl;
//	for (int is = 0; is < n_vars; is++)
//	{
//		//std::cout << "variable " << var_names[iforc] << std::endl;
//		// get variable id
//		if ((retncval = nc_inq_varid(ncid, (const char*)var_names[is].c_str(), &v_varid)))
//			ncERR(retncval, var_names[is]);
//		//read variable (input data)	
//		if ((retncval = nc_get_vara_float(ncid, v_varid, start, count, &forcVar[is][0][0])))
//			ncERR(retncval, " reading "+ var_names[is]);
//	}
//	//close file
//	if ((retncval = nc_close(ncid)))
//		ncERR(retncval, FileName);
//
//	if (myrank == 0) {
//		std::cout << "read forcing time " << MPI::Wtime() - interm_time << std::endl;
//		interm_time = MPI::Wtime();
//	}
//
//	for (int it = 0; it < num_time_cycle; it++) {
//
//		Generate_Samples_and_Cov(myrank, it, interm_time);  //, multivarNormalDistSamplesForc);
//
//		for (int ie = 0; ie < ens_size; ie++) {
//			for (int is = 0; is < n_vars; is++) {
//				if (multi_bool[is] == 1) {
//					for (int ig = 0; ig < tot_gridSize; ig++) {
//						outArray[ie][is][it][ig] = forcVar[is][it][ig] *
//							(1.0 + std_dev[is] * _multivarNormalDistSamplesForc[is](ig, ie));
//					}
//				}
//				else {
//					for (int ig = 0; ig < tot_gridSize; ig++) {
//						outArray[ie][is][it][ig] = forcVar[is][it][ig] +
//							(std_dev[is] * _multivarNormalDistSamplesForc[is](ig, ie));
//					}
//				}
//			}
//		}
//	}
//	
//	if (myrank == 0) {
//		std::cout << " Generate random samples time " << MPI::Wtime() - interm_time << std::endl;
//		interm_time = MPI::Wtime();
//	}
//	MPI::COMM_WORLD.Barrier();
//
//
//	char eStr[2];
//	for (int ie = 0; ie < ens_size; ie++) {
//
//		sprintf(eStr, "%02d", ie + 1);
//		FileName = FileDir + "/ens" + eStr + "/" + forc_inp_file; //std::to_string(ie+1)
//		//Open the netcdf file.  
//		//if ((retncval = nc_open((const char*)FileName.c_str(), NC_WRITE, &ncid)))
//		//	ncERR(retncval, " open " + FileName);  // , FILE_NAME);
//
//		if ((retncval = nc_open_par((const char*)FileName.c_str(), NC_WRITE,
//			MPI::COMM_WORLD, MPI::INFO_NULL, &ncid)))
//			ncERR(retncval, " open " + FileName);  // , FILE_NAME);
//
//		//set collective I / O globally(for all variables)
//		//if ((retncval = nc_var_par_access(ncid, NC_GLOBAL, NC_COLLECTIVE)))
//		//	ncERR(retncval, " Setting collective write");  // , FILE_NAME);
//
//		//std::cout << "starting loop " << std::endl;		
//		for (int is = 0; is < n_vars; is++) {
//	
//			//std::cout << "variable " << var_names[iforc] << std::endl;
//			// get variable id
//			if ((retncval = nc_inq_varid(ncid, (const char*)var_names[is].c_str(), &v_varid)))
//				ncERR(retncval, var_names[is]);
//
//			if ((retncval = nc_var_par_access(ncid, v_varid, NC_COLLECTIVE)))
//				ncERR(retncval, " Setting collective write");  // , FILE_NAME);
//
//			if ((retncval = nc_put_vara_float(ncid, v_varid, start, count, &outArray[ie][is][0][0])))
//				ncERR(retncval, " writing " + var_names[is]);
//		}
//	}
//	
//	//close file
//	if ((retncval = nc_close(ncid)))
//		ncERR(retncval, " close "+ FileName);
//
//	
//	for (int is = 0; is < n_vars; is++) {
//		for (int it = 0; it < num_time_cycle; it++)
//		{
//			delete[] forcVar[is][it];
//			forcVar[is][it] = NULL;
//		}
//		delete[] forcVar[is];
//		forcVar[is] = NULL;
//	}
//	delete[] forcVar;
//	forcVar = NULL; 
//
//	for (int ie = 0; ie < ens_size; ie++) {
//		for (int is = 0; is < n_vars; is++) {
//			for (int it = 0; it < num_time_cycle; it++) {
//				delete[] outArray[ie][is][it];
//				outArray[ie][is][it] = NULL;
//			}
//			delete[] outArray[ie][is];
//			outArray[ie][is] = NULL;
//		}
//		delete[] outArray[ie];
//		outArray[ie] = NULL; 
//	}
//	delete[] outArray;
//	outArray = NULL; 
//	
//	/*if (myrank == 0) {
//		for (int it = 0; it < num_time_cycle; it++) {
//			delete[] out_array_put[it];
//			out_array_put[it] = NULL;
//		}
//		delete[] out_array_put;
//		out_array_put = NULL;
//	}*/
//
//	if (myrank == 0) {
//		std::cout << "Generate and write ens forc time " << MPI::Wtime() - interm_time << std::endl;
//		interm_time = MPI::Wtime();
//	}
//	return;
//}

//void modelGridCells::Generate_Samples_and_Cov(int myrank, int istep, double &interm_time) //= 0
//{
//	/*float std_dev[8] = { 0.1, 0.05, 20.0, 2.0, 0.05, 0.05, 0.1, 0.01 };
//	float mean_var[8] = { 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0 };
//	int multi_bool[8] = { 1, 1, 0, 0, 1, 1, 1, 1 };*/
//
//	//, RowMajor
//	std::vector<Eigen::Matrix<float, Dynamic, Dynamic>>
//		multivarNormalDistSamplesForc(n_vars,
//			//, RowMajor
//			Eigen::Matrix<float, Dynamic, Dynamic>(tot_gridSize, ens_size));
//	for (int is = 0; is < 7; is++)
//		multivarNormalDistSamplesForc[is] = std_norm_dist_Forc_Default.samples(ens_size);
//	//apply L to prec, Qsi, Qli
//	multivarNormalDistSamplesForc[2] = corrFs_L(2, 0) * multivarNormalDistSamplesForc[0]
//		+ corrFs_L(2, 1) * multivarNormalDistSamplesForc[1]
//		+ corrFs_L(2, 2) * multivarNormalDistSamplesForc[2];
//	multivarNormalDistSamplesForc[1] = corrFs_L(1, 0) * multivarNormalDistSamplesForc[0]
//		+ corrFs_L(1, 1) * multivarNormalDistSamplesForc[1];
//	//+ corrFs_L(1,2) * multivarNormalDistSamplesForc[2]; // corrFs_L(1,2) = 0.
//	for (int is = 0; is < n_vars; is++) {
//		std::ofstream stdnormalSamplestxt("ens_err_" + var_names[is]
//			+ "_rank" + std::to_string(myrank) + ".txt");
//		stdnormalSamplestxt << multivarNormalDistSamplesForc[is] << std::endl;
//	}
//	if (myrank == 0) {
//		std::cout << " sample generate time " << MPI::Wtime() - interm_time << std::endl;
//		interm_time = MPI::Wtime();
//	}
//
//	if (ini_time == 0) {   //ini_time = 0 >> no stored rand pattern
//		for (int is = 0; is < n_vars; is++) {
//			_multivarNormalDistSamplesForc[is] = multivarNormalDistSamplesForc[is];
//		}
//	}
//	else if (istep == 0)   //new day
//	{
//		std::vector<Eigen::Matrix<float, Dynamic, Dynamic>> //, RowMajor
//			multivarNormalDistSamplesForc_in(n_vars,
//				Eigen::Matrix<float, Dynamic, Dynamic>(tot_gridSize, ens_size)); //, RowMajor
//
//		Read_random_samples(tot_gridSize, ens_size, multivarNormalDistSamplesForc_in,
//			"ens_err_rank" + std::to_string(myrank) + ".nc", var_names);
//		for (int is = 0; is < n_vars; is++) {
//			_multivarNormalDistSamplesForc[is] = corrFact1 * multivarNormalDistSamplesForc_in[is]
//				+ corrFact2 * multivarNormalDistSamplesForc[is];
//		}
//	}
//	else {
//		for (int is = 0; is < n_vars; is++) {
//			_multivarNormalDistSamplesForc[is] = corrFact1 * _multivarNormalDistSamplesForc[is]
//				+ corrFact2 * multivarNormalDistSamplesForc[is];
//		}
//	}
//	if (myrank == 0) {
//		std::cout << "temporal cor applied " << MPI::Wtime() - interm_time << std::endl;
//		interm_time = MPI::Wtime();
//	}
//	float fillVal = -999999.9999;
//	if (istep + 1 >= num_time_cycle) {
//		write_random_samples(tot_gridSize, ens_size,
//			_multivarNormalDistSamplesForc,
//			"ens_err_rank" + std::to_string(myrank) + ".nc", var_names, &fillVal);
//
//		if (myrank == 0) {
//			std::cout << "write samples time " << MPI::Wtime() - interm_time << std::endl;
//			interm_time = MPI::Wtime();
//		}
//	}
//
//	return;
//}

void modelGridCells::Create_Ensemble_Samples(int myrank, int num_samples,
	std::string FileDir, std::string forc_inp_file, double &interm_time)
{
	int ncid, v_varid;   // = 0;     // , t_varid = 0;
	// Error handling.  
	int retncval = 0;
	size_t start[2], count[2];
	start[1] = 0;
	count[0] = 1;  // num_time_cycle;
	count[1] = tot_gridSize;
	//float*** forcVar = new float**[n_vars];
	//for (int is = 0; is < n_vars; is++) {
	//	forcVar[is] = new float*[num_time_cycle];
	//	for (int it = 0; it < num_time_cycle; it++)
	//	{
	//		forcVar[is][it] = new float[tot_gridSize];
	//	}
	//}
	//float*** outArray = new float**[n_vars];
	//for (int is = 0; is < n_vars; is++) {
	//	outArray[is] = new float*[num_time_cycle];
	//	for (int it = 0; it < num_time_cycle; it++) {
	//		outArray[is][it] = new float[tot_gridSize];
	//	}
	//}
	float* forcVar = new float[tot_gridSize];
	float* outArray = new float[tot_gridSize];
	std::cout << "proc " << myrank << " array alloc " << std::endl;
	std::string FileName;
	// read inputs
	char eStr[3];
	
	/*if (myrank == 0) {
		std::cout << "read forcing time " << MPI::Wtime() - interm_time << std::endl;
		interm_time = MPI::Wtime();
	}*/
	std::string InitFileName = FileDir + "/ens_anom_init.nc";
	for (int it = 0; it < num_time_cycle; it++){
		std::cout << "proc " << myrank << " time " << it << " ens loop ";
		start[0] = it;
		Generate_Samples_and_Cov(myrank, it, num_samples, InitFileName, interm_time);  //, multivarNormalDistSamplesForc);
		//std::cout << "proc " << myrank << " generated samples at " <<it<< std::endl;
		for (int ie = 0; ie < num_samples; ie++) {
			std::cout << ie << " ";
			sprintf(eStr, "%02d", ie + 1);
			FileName = FileDir + "/ens" + eStr + "/" + forc_inp_file; //std::to_string(ie+1)
			//Open the netcdf file.  
			if ((retncval = nc_open((const char*)FileName.c_str(), NC_WRITE, &ncid)))
				ncERR(retncval, " opening " + FileName);
			//std::cout << "starting loop " << std::endl;
			//std::cout << "proc " << myrank << " open forcing file " << FileName << std::endl;

			for (int is = 0; is < n_vars; is++) {
				//std::cout << "variable " << var_names[iforc] << std::endl;
			// get variable id
				if ((retncval = nc_inq_varid(ncid, (const char*)var_names[is].c_str(), &v_varid)))
					ncERR(retncval, var_names[is]);
				//read variable (input data)	
				if ((retncval = nc_get_vara_float(ncid, v_varid, start, count, &forcVar[0])))
					ncERR(retncval, " reading " + var_names[is]);

				//std::cout << "proc " << myrank << " read forcing " << var_names[is] << std::endl;

				if (multi_bool[is] == 1) {
					for (int ig = 0; ig < tot_gridSize; ig++) {
						outArray[ig] = forcVar[ig] *
							(1.0 + std_dev[is] * _multivarNormalDistSamplesForc[is](ig, ie));
					}
				}
				else {
					for (int ig = 0; ig < tot_gridSize; ig++) {
						outArray[ig] = forcVar[ig] +
							(std_dev[is] * _multivarNormalDistSamplesForc[is](ig, ie));
					}
				}
				//std::cout << "proc " << myrank << " read forcing " << var_names[is]<< std::endl;

				if ((retncval = nc_put_vara_float(ncid, v_varid, start, count, &outArray[0])))
					ncERR(retncval, " writing " + var_names[is]);
			}
			//close file
			if ((retncval = nc_close(ncid)))
				ncERR(retncval, " closing " + FileName);
		}
		std::cout << std::endl;
	}
	
	/*if (myrank == 0) {
		std::cout << " Generate random samples time " << MPI::Wtime() - interm_time << std::endl;
		interm_time = MPI::Wtime();
	}*/
	
	/*for (int is = 0; is < n_vars; is++) {
		for (int it = 0; it < num_time_cycle; it++)
		{
			delete[] forcVar[is][it];
			forcVar[is][it] = NULL;
		}
		delete[] forcVar[is];
		forcVar[is] = NULL;
	}*/
	delete[] forcVar;
	forcVar = NULL;
	/*for (int is = 0; is < n_vars; is++) {
		for (int it = 0; it < num_time_cycle; it++)
		{
			delete[] outArray[is][it];
			outArray[is][it] = NULL;
		}
		delete[] outArray[is];
		outArray[is] = NULL;
	}*/
	delete[] outArray;
	outArray = NULL;

	std::cout << "proc " << myrank << " wrote ens forcing " << std::endl;

	float fillVal = -999.999;
	write_random_samples(tot_gridSize, num_samples, n_vars,
		_multivarNormalDistSamplesForc, InitFileName, var_names, &fillVal);

	//std::cout << "proc " << myrank << " wrote ens anom init pattern " << std::endl;

	/*if (myrank == 0) {
		std::cout << "write samples time " << MPI::Wtime() - interm_time << std::endl;
		interm_time = MPI::Wtime();
	}*/
	/*if (myrank == 0) {
		std::cout << "Write ens forc time " << MPI::Wtime() - interm_time << std::endl;
		interm_time = MPI::Wtime();
	}*/
	return;
}

void modelGridCells::Generate_Samples_and_Cov(int myrank, int istep, 
	int num_samples, std::string FileName, double &interm_time) //= 0
{
	/*float std_dev[8] = { 0.1, 0.05, 20.0, 2.0, 0.05, 0.05, 0.1, 0.01 };
	float mean_var[8] = { 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0 };
	int multi_bool[8] = { 1, 1, 0, 0, 1, 1, 1, 1 };*/

	//, RowMajor
	std::vector<Eigen::Matrix<float, Dynamic, Dynamic>>
		multivarNormalDistSamplesForc(n_vars,
			//, RowMajor
			Eigen::Matrix<float, Dynamic, Dynamic>(tot_gridSize, num_samples));
	for (int is = 0; is < n_vars; is++)
		multivarNormalDistSamplesForc[is] = std_norm_dist_Forc_Default.samples(num_samples);

	//apply L to prec, Qsi, Qli
	multivarNormalDistSamplesForc[2] = corrFs_L(2, 0) * multivarNormalDistSamplesForc[0]
		+ corrFs_L(2, 1) * multivarNormalDistSamplesForc[1]
		+ corrFs_L(2, 2) * multivarNormalDistSamplesForc[2];
	multivarNormalDistSamplesForc[1] = corrFs_L(1, 0) * multivarNormalDistSamplesForc[0]
		+ corrFs_L(1, 1) * multivarNormalDistSamplesForc[1];
	
	// use same factors for precip conerve and precip bilinear
	//multivarNormalDistSamplesForc[6] = multivarNormalDistSamplesForc[0];
	//+ corrFs_L(1,2) * multivarNormalDistSamplesForc[2]; // corrFs_L(1,2) = 0.
	/*for (int is = 0; is < n_vars; is++) {
		std::ofstream stdnormalSamplestxt("ens_err_" + var_names[is]
			+ "_rank" + std::to_string(myrank) + ".txt");
		stdnormalSamplestxt << multivarNormalDistSamplesForc[is] << std::endl;
	}
	if (myrank == 0) {
		std::cout << " sample generate time " << MPI::Wtime() - interm_time << std::endl;
		interm_time = MPI::Wtime();
	}*/

	if (ini_time == 0) {   //ini_time = 0 >> no stored rand pattern
		for (int is = 0; is < n_vars; is++) {
			_multivarNormalDistSamplesForc[is] = multivarNormalDistSamplesForc[is];
		}
		ini_time = ini_time + 1;
	}
	else if (istep == 0)   //new day
	{
		std::vector<Eigen::Matrix<float, Dynamic, Dynamic>> //, RowMajor
			multivarNormalDistSamplesForc_in(n_vars,
				Eigen::Matrix<float, Dynamic, Dynamic>(tot_gridSize, num_samples)); //, RowMajor
		
		Read_random_samples(tot_gridSize, num_samples, n_vars,
			multivarNormalDistSamplesForc_in, FileName, var_names);

		for (int is = 0; is < n_vars; is++) {
			_multivarNormalDistSamplesForc[is] = corrFact1 * multivarNormalDistSamplesForc_in[is]
				+ corrFact2 * multivarNormalDistSamplesForc[is];
		}
	}
	else {
		for (int is = 0; is < n_vars; is++) {
			_multivarNormalDistSamplesForc[is] = corrFact1 * _multivarNormalDistSamplesForc[is]
				+ corrFact2 * multivarNormalDistSamplesForc[is];
		}
	}
	/*if (myrank == 0) {
		std::cout << "temporal cor applied " << MPI::Wtime() - interm_time << std::endl;
		interm_time = MPI::Wtime();
	}*/

	return;
}




