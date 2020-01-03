//Program For Calibration for Swaptions (G2 Model + HULL) 


#include <ql/qldefines.hpp>
#ifdef BOOST_MSVC
#  include <ql/auto_link.hpp>
#endif
#include <ql/instruments/swaption.hpp>
#include <ql/pricingengines/swap/discountingswapengine.hpp>
#include <ql/pricingengines/swaption/jamshidianswaptionengine.hpp>
#include <ql/pricingengines/swaption/g2swaptionengine.hpp>
#include <ql/pricingengines/swaption/fdhullwhiteswaptionengine.hpp>
#include <ql/models/shortrate/calibrationhelpers/swaptionhelper.hpp>
#include <ql/models/shortrate/onefactormodels/blackkarasinski.hpp>
#include <ql/math/optimization/levenbergmarquardt.hpp>
#include <ql/indexes/ibor/euribor.hpp>
#include <ql/cashflows/coupon.hpp>
#include <ql/quotes/simplequote.hpp>
#include <ql/termstructures/yield/flatforward.hpp>
#include <ql/time/calendars/target.hpp>
#include <ql/time/daycounters/thirty360.hpp>
#include <ql/utilities/dataformatters.hpp>
#include <iostream>


using namespace QuantLib;
using namespace std;


Size numRows = 5;
Size numCols = 5;

Integer swapLenghts[] = {
	1,     2,     3,     4,  5};

Volatility swaptionVols[] =
{
	0.1640, 0.1550, 0.1430, 0.1310, 0.1240,
	0.1600, 0.1500, 0.1390, 0.1290, 0.1220,
	0.1570, 0.1450, 0.1340, 0.1240, 0.1190,
	0.1480, 0.1360, 0.1260, 0.1190, 0.1140,
	0.1400, 0.1280, 0.1210, 0.1140, 0.1100,
	0.1300, 0.1190, 0.1130, 0.1050, 0.1010,
	0.1160, 0.1070, 0.1000, 0.0930, 0.0900
};


void calibrateSwaption(
	const ext::shared_ptr<ShortRateModel>& model,
	const std::vector<ext::shared_ptr<BlackCalibrationHelper> >& helpers) {

	LevenbergMarquardt om;
	model->calibrate(helpers, om,
		EndCriteria(400, 100, 1.0e-8, 1.0e-8, 1.0e-8));

	
	for (Size i = 0; i<helpers.size(); i++) {
		Real npv = helpers[i]->modelValue();
		Volatility implied = helpers[i]->impliedVolatility(npv, 1e-4, 1000, 0.05, 0.50);
		Volatility diff = implied - swaptionVols[i];

		std::cout << std::setprecision(5) << std::noshowpos
			<< ": model " << std::setw(7) << io::volatility(implied)
			<< ", market " << std::setw(7)
			<< io::volatility(swaptionVols[i])
			<< " (" << std::setw(7) << std::showpos
			<< io::volatility(diff) << std::noshowpos << ")\n";
	}
}

int main(int, char*[]) 
{
		Date todaysDate(10, February, 2016);
		Calendar calendar = TARGET();
		Date settlementDate(17, February, 2016);
		Settings::instance().evaluationDate() = todaysDate;

		
		ext::shared_ptr<Quote> flatRate(new SimpleQuote(0.06215));
		Handle<YieldTermStructure> rhTermStructure(ext::make_shared<FlatForward>(settlementDate, Handle<Quote>(flatRate),Actual365Fixed()));
		ext::shared_ptr<IborIndex> indexSixMonths(new Euribor6M(rhTermStructure));

		std::vector<Period> swaptionMaturities;
		swaptionMaturities.push_back(Period(1, Years));
		swaptionMaturities.push_back(Period(2, Years));
		swaptionMaturities.push_back(Period(3, Years));
		swaptionMaturities.push_back(Period(4, Years));
		swaptionMaturities.push_back(Period(5, Years));

		std::vector<ext::shared_ptr<BlackCalibrationHelper> > swaptions;


		Size i,j;
		for (Size i = 0; i<numRows; i++)
		{
			for (Size j = 0; j < numCols; j++)
			{
				Size k = i*numCols + j;
				ext::shared_ptr<Quote> vol(new SimpleQuote(swaptionVols[k]));
				
				swaptions.push_back(ext::shared_ptr<BlackCalibrationHelper>(new
					SwaptionHelper(swaptionMaturities[i],
						Period(swapLenghts[j], Years),
						Handle<Quote>(vol),
						indexSixMonths,
						indexSixMonths->tenor(),
						indexSixMonths->dayCounter(),
						indexSixMonths->dayCounter(),
						rhTermStructure)));
			}
		}

		ext::shared_ptr<G2> modelG2(new G2(rhTermStructure));
		ext::shared_ptr<HullWhite> modelHW(new HullWhite(rhTermStructure));
		ext::shared_ptr<HullWhite> modelHW2(new HullWhite(rhTermStructure));
		


		// model calibrations

		std::cout << "G2 (analytic formulae) calibration" << std::endl;
		for (i = 0; i<swaptions.size(); i++)
			swaptions[i]->setPricingEngine(ext::shared_ptr<PricingEngine>(new G2SwaptionEngine(modelG2, 6.0, 16)));

		calibrateSwaption(modelG2, swaptions);
		std::cout << "calibrated to:\n"
			<< "a     = " << modelG2->params()[0] << ", "
			<< "sigma = " << modelG2->params()[1] << "\n"
			<< "b     = " << modelG2->params()[2] << ", "
			<< "eta   = " << modelG2->params()[3] << "\n"
			<< "rho   = " << modelG2->params()[4]
			<< std::endl << std::endl;



		std::cout << "Hull-White (analytic formulae) calibration" << std::endl;
		for (i = 0; i<swaptions.size(); i++)
			swaptions[i]->setPricingEngine(ext::shared_ptr<PricingEngine>(new JamshidianSwaptionEngine(modelHW)));

		calibrateSwaption(modelHW, swaptions);
		std::cout << "calibrated to:\n"<< "a = " << modelHW->params()[0] << ", "<< "sigma = " << modelHW->params()[1]<< std::endl << std::endl;

		return 0;
}

