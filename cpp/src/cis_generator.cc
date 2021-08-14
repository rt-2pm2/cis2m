#include "cis_generator.hpp"
#include <iostream>


namespace cis2m {
	// Helper Functions
	MatrixXd blkdiag(const MatrixXd& A, int count) {
		MatrixXd bdm = MatrixXd::Zero(A.rows() * count, A.cols() * count);
		for (int i = 0; i < count; ++i) {
			bdm.block(i * A.rows(), i * A.cols(), A.rows(), A.cols()) = A;
		}
		return bdm;
	}


	// ========================================================================
	// CLASS
	CISGenerator::CISGenerator(const MatrixXd& Ad, const MatrixXd& Bd) {
		StateDim_ = Ad.rows();
		NumberInputs_ = Bd.cols();
		DisturbanceDim_ = 0;

		GenerateBrunovksyForm(Ad, Bd);
		
		Level_ = -1;
		Transient_ = -1;
		
		cis_computed_ = false;
	}


	CISGenerator::CISGenerator(const MatrixXd& Ad, const MatrixXd& Bd, const MatrixXd& Ed) {

		Ed_ = Ed;
		StateDim_ = Ad.rows();
		NumberInputs_ = Bd.cols();
		DisturbanceDim_ = Ed.cols();

		GenerateBrunovksyForm(Ad, Bd);

		cis_computed_ = false;
	}


	CISGenerator::~CISGenerator() {};

	void CISGenerator::GenerateBrunovksyForm(const MatrixXd& A, const MatrixXd& B) {
		brunovsky_form_ = new BrunovskyForm(A, B);  
		
	}

	void CISGenerator::AddDisturbanceSet(const HPolyhedron& ds) {
		DisturbanceSet_ = ds; 
	}

		
	void CISGenerator::AddInputConstraintsSet(const HPolyhedron& ics) {
		InputCnstrSet_ = ics;
	}


	std::vector<HPolyhedron> CISGenerator::ComputeShrinkedSafeSetsSequence(const HPolyhedron& ss) {

		int nmax = brunovsky_form_->GetMaxControllabilityIndex();
		std::pair<MatrixXd, MatrixXd> pairAB = brunovsky_form_->GetDynSystem();
		MatrixXd Ad_BF = pairAB.first; 
		HPolyhedron SafeSet_BF = brunovsky_form_->GetDynConstraints(ss);

		std::vector<HPolyhedron> SafeSet_seq;
				
		if (Ed_.size() > 0) {
			MatrixXd DynMat = MatrixXd::Identity(StateDim_, StateDim_);
			for (int i = 0; i < nmax; i++) {
				SafeSet_seq.push_back(SafeSet_BF - DisturbanceSet_.affineT(DynMat * Ed_));
				DynMat *= Ad_BF;
			}
		} else {
			SafeSet_seq.push_back(SafeSet_BF);
		}

#ifdef CIS2M_DEBUG
		std::cout << __FILE__ << std::endl;
		std::cout << "Computation of Safeset sequence" << std::endl;
		std::cout << "SafeSet Base A: " << std::endl << SafeSet_BF.Ai() << std::endl;
		std::cout << "SafeSet Base B: " << std::endl << SafeSet_BF.bi() << std::endl;
		std::cout << std::endl;
#endif


		return SafeSet_seq;
		
	}


	void CISGenerator::ComputeLiftedSystem(int L, int T) {
		// Update the parameters	
		Level_ = L;
		Transient_ = T;
		int length = Transient_ + Level_;

		// Construct the High-Dimensional system
		MatrixXd Ki(MatrixXd::Zero(1, length));
		Ki(0) = 1.0;
		MatrixXd Pi(MatrixXd::Zero(length, length));
		Pi.block(0, 1, length - 1, length - 1) = MatrixXd::Identity(length - 1, length - 1);
		MatrixXd K = blkdiag(Ki, NumberInputs_);
		MatrixXd P = blkdiag(Pi, NumberInputs_); 

		int Nrow_hd = StateDim_ + length  * NumberInputs_;
		int Ncol_hd = StateDim_ + P.cols();
		MatrixXd Ahd(MatrixXd::Zero(Nrow_hd, Ncol_hd));

		std::pair<MatrixXd, MatrixXd> pairAB = brunovsky_form_->GetDynSystem();
		MatrixXd Ad_BF = pairAB.first; 
		MatrixXd Bd_BF = pairAB.second;
		Ahd.block(0, 0, StateDim_, StateDim_) = Ad_BF;
		Ahd.block(0, StateDim_, StateDim_, K.cols()) = Bd_BF * K;
		Ahd.block(StateDim_, StateDim_, P.rows(), P.cols()) = P;
		
#ifdef CIS2M_DEBUG
		std::cout << __FILE__ << std::endl;
		std::cout << "Computation of Lifted System" << std::endl;
		std::cout << "A_BF: " << std::endl << Ad_BF << std::endl;
		std::cout << "B_BF: " << std::endl << Bd_BF << std::endl;
		std::cout << "A_lifted: " << std::endl << Ahd << std::endl;
		std::cout << "A_lifted Size: " << Ahd.rows() << " x " << Ahd.cols() << std::endl;
		std::cout << std::endl;
#endif

		A_lifted_ = Ahd;
	}


	HPolyhedron CISGenerator::computeCIS(const HPolyhedron& SafeSet, int L, int T) {
		if (Level_ != L || Transient_ != T)
			ComputeLiftedSystem(L, T);

		int length = Level_ + Transient_;
		
		int mu_max = brunovsky_form_->GetMaxControllabilityIndex();
		HPolyhedron polyhedron_Gb = brunovsky_form_->GetDynConstraints(SafeSet);
		int NDynconstr = polyhedron_Gb.Ai().rows();

		int mcisA_rows = NDynconstr * (mu_max + length); 
		int mcisA_cols = StateDim_ + NumberInputs_ * length;
		MatrixXd mcisA(MatrixXd::Zero(mcisA_rows, mcisA_cols));
		MatrixXd mcisb(MatrixXd::Zero(mcisA_rows, 1)); 

		mcisA.block(0, 0, NDynconstr, StateDim_) = polyhedron_Gb.Ai(); 
		mcisb.block(0, 0, NDynconstr, 1) = polyhedron_Gb.bi();

		std::vector<HPolyhedron> seq = ComputeShrinkedSafeSetsSequence(SafeSet);

		MatrixXd Acurr = A_lifted_; 
		for (int t = 0; t < mu_max + (length - 1); t++) {
			int tbar = t < seq.size() ? t : seq.size() - 1;
			MatrixXd TempA(MatrixXd::Zero(NDynconstr, NumberInputs_ * length + StateDim_));
			TempA.block(0, 0, NDynconstr, StateDim_) = seq[tbar].Ai();

			mcisA.block(NDynconstr * t, 0, NDynconstr, mcisA_cols) = TempA * Acurr; 
			mcisb.block(NDynconstr * t, 0, NDynconstr, 1) = seq[tbar].bi();
			Acurr *= A_lifted_;
		}

		// Get the transformation
		MatrixXd Transform = brunovsky_form_->GetTransformationMatrix();

		std::cout << "Transf: " << std::endl << Transform << std::endl;

		mcisA.block(0, 0, mcisA.rows(), StateDim_) = mcisA.block(0, 0, mcisA.rows(), StateDim_) * 
			Transform;

		// Note: The CIS is expressed in the original basis
		CIS_ =  HPolyhedron(mcisA, mcisb);
		cis_computed_ = true;
		return CIS_;
	}

	std::optional<HPolyhedron> CISGenerator::FetchCIS() {
		if (cis_computed_) {
			return CIS_;
		}
		else{
			return {};	
		}
	}

}
