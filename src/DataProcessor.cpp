#include "DataProcessor.h"
#include <cmath>

using namespace std;

/**
 * Constructor, initializes the DataProcessor object.
 * 
 * @brief Constructor
 * 
 * @author Stefan
 * @date June 20, 2016
 * @version 0.5
 */
DataProcessor::DataProcessor()
{
}

/**
 * Destructor, frees the allocated memory for this object, when it is
 * destroyed
 * 
 * @brief Destructor
 * 
 * @author Stefan
 * @date May 30, 2016
 * @version 0.1
 */
DataProcessor::~DataProcessor()
{
	//TODO implement
}

/**
 * Computes the integral of the data given as parameter. It will count positive as well as
 * negative entries and sum up the bin content.
 *
 * @author Bene9
 * @date June 15, 2016
 * @version 0.2
 *
 * @param data TH1 family object containing the data to be integrated.
 * @return Value of the integral over all bins
 */
Double_t DataProcessor::computeIntegral(TH1D& data)
{
	//Double_t nBins;
	Double_t counter = 0;

	//nBins = data.GetNbinsX();

	//TODO check if parallelization is of use here

	for (int i = 0; i < data.GetNbinsX(); i++)
	{
		counter += data.GetBinContent(i);
	}

	return counter * data.GetBinWidth(1);
}

/**
 * Integrates the given data, which is in a histogram. This method will return
 * a new histogram of type TH1* containing the integrated original data.
 * 
 * @brief histogram integrator
 * 
 * @author Stefan
 * @date June 16, 2016
 * @version 0.3
 * 
 * @param data Data, that is to be integrated
 * 
 * @return TH1D* pointer to a new heap-object histogram containing the integral of data
 * 
 * @warning Does integrate the whole interval, that the data object provides data.
 * @warning Returned object must be destroyed by the user
 */
TH1D* DataProcessor::integrate(TH1D* data)
{
	Int_t nBins = data->GetNbinsX();
	Double_t* binLowEdges = new Double_t[nBins + 1];

	for (Int_t i = 0; i <= nBins; i++)
	{
		binLowEdges[i] = data->GetBinLowEdge(i);
	}

	//TODO Check the actual parameter type and and return the correct object accordingly
	TH1D* result = new TH1D("not final", "yet", nBins, binLowEdges);
	result->GetXaxis()->SetTitle(data->GetXaxis()->GetTitle());
	stringstream yTitle;
	yTitle << "integral of " << data->GetYaxis()->GetTitle();
	result->GetYaxis()->SetTitle(yTitle.str().c_str());

	delete binLowEdges;
	//choose random bin width since all bins are the same size - might change
	Double_t binWidth = result->GetBinWidth(1);

	double integral = 0.0;

	for (int i = 0; i <= nBins; i++)
	{
		integral += data->GetBinContent(i) * binWidth;
		result->Fill(data->GetBinLowEdge(i), integral);
	}

	return result;
}

/**
 * Integrates all histograms, that are passed to this method in form of an array of
 * pointers to TH1 type objects. The resulting objects are written to the passed array
 * storage
 *
 * @brief Integrate all data in an array
 *
 * @param data Pointer to the DataSet object that is to be integrated
 *
 * @return Pointer to a DataSet type object containing the processed data
 *
 * @warning Heap object returned, caller needs to handle memory
 */
DataSet* DataProcessor::integrateAll(DataSet* data)
{
	DataSet* result = new DataSet();
	//TODO check why element data[size-1] is already corrupted
	for(int i = 0; i < data->getSize(); i++)
	{
		cout << "[" << i + 1 << "/" << data->getSize() << "] integrating" << endl;
		try
		{
			TH1D* integral = integrate(data->getEvent(i));
			result->addData(integral);
		}
		catch(Exception& e)
		{
			cerr << e.error() << endl;
		}
	}

	return result;
}


/**
 * Find the position of the minimum of the given data. Will only find the absolute
 * minimum. Can not yet find more than one negative peak.
 *
 * @author Stefan
 * @date July 2, 2016
 * @version 0.1
 *
 * @param data Data as a pointer to a TH1D histogram containing the data
 * @return Center position of the bin containing the data minimum
 */
double DataProcessor::findMinimum(TH1D* data)
{
	int bin = data->GetMinimumBin();
	return data->GetBinCenter(bin);
}


TH1D* DataProcessor::calculateDriftTimeSpectrum(DataSet* data)
{
	//TODO fully implement and test
	//TODO parallel?

	double triggerpos = 42;
	TH1D* result = new TH1D("Drifttime spectrum","TDC spectrum",150,0,100);

	#pragma omp parallel for
	for(int i = 0; i < data->getSize(); i++)
	{
		TH1D* event = data->getEvent(i);
		double diff  = triggerpos - findMinimum(event);
		result->Fill(diff);
	}

	return result;
}
