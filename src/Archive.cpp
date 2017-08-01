/*
 *
 *  Created on: 15.06.2016
 *      Author: stefan
 */

#include "Archive.h"

using namespace std;

/**
 * Constructor, initializes the Archive object.
 *
 * @brief Constructor
 *
 * @author Stefan Bieschke
 * @date July 17, 2017
 * @version Alpha 2.0
 *
 * @param filename relative path to the .drift-file containing the raw data
 */
Archive::Archive(string filename)
{
	convertAllEntries(filename);

	m_directory = parseDir(filename);
	m_file = parseFile(filename);
}

/**
 * Destructor, that MUST be called when destroying the archive. Destructor
 * calls a function to write the archives content to disk.
 *
 * @brief Dtor
 *
 * @author Stefan
 * @date June 28,2016
 * @version 0.1
 */
Archive::~Archive()
{
	stringstream name;
	name << m_directory << "processed_" << m_file;
	string filename(name.str());
	cout << "Saving data to: " << filename << endl;
//	writeToFile(filename);
}

/**
 * Returns the filename in a string.
 *
 *
 * @brief Getter method for the filename
 *
 * @author Stefan Bieschke
 * @date July 17, 2017
 * @version Alpha 2.0
 *
 * @return string containing filename
 */
const string& Archive::getFilename() const
{
	return m_file;
}

/**
 * Returns the directory name, where the original, raw data is.
 *
 * @brief Getter method for the directory name
 *
 * @author Stefan Bieschke
 * @date July 17, 2017
 * @version Alpha 2.0
 *
 * @return string containing directory name
 */
const string& Archive::getDirname() const
{
	return m_directory;
}


/**
 * Getter method for the Drifttubes. Returns a vector containing unique pointers to the drift tubes.
 *
 * @brief Drifttubes getter.
 *
 * @author Stefan Bieschke
 * @date July 17, 2017
 * @version Alpha 2.0
 *
 * @return Const reference to the vector containing unique_ptr<Drifttube> objects.
 */
const vector<unique_ptr<Drifttube>>& Archive::getTubes() const
{
	return m_tubes;
}

//TODO test
/**
 * Writes the results and data to a file, that is specified with parameter filename.
 *
 * @brief Write data to file
 *
 * @author Stefan Bieschke
 * @date July 20, 2017
 * @version Alpha 2.0
 *
 * @param filename relative path to the file.
 *
 * @warning Can overwrite existing files, handle with care.
 */
void Archive::writeToFile(const string& filename)
{
	ofstream file(filename, ios::out | ios::binary);

	//write header... must contain: nTubes, nEvents per tube,...
	//TODO complete impl
	size_t nTubes = m_tubes.size();
	size_t nEvents = m_tubes[0]->getDataSet().getSize() - m_tubes[0]->getDriftTimeSpectrum().getRejected();
	size_t nEventSize = 800;
	file.write((char*)&nTubes,sizeof(size_t));
	file.write((char*)&nEvents,sizeof(size_t));
	file.write((char*)&nEventSize,sizeof(size_t));

	//loop over tubes
	for(size_t i = 0; i < m_tubes.size(); i++)
	{
		//loop over DataSet for each tube
		for(size_t j = 0; j < m_tubes[i]->getDataSet().getSize(); j++)
		{
			try
			{
				Event e = m_tubes[i]->getDataSet()[j];
				array<int,800> integral = DataProcessor::integrate(e);
				//write eventnumber
				file.write((char*)&j,sizeof(size_t));
				//write event
				for(size_t k = 0; k < e.getData().size(); k++)
				{
					double datum = (e[k] - OFFSET_ZERO_VOLTAGE) * ADC_CHANNELS_TO_VOLTAGE;
					file.write((char*)&datum,sizeof(double));
				}
				//wirte integral
				for(size_t k = 0; k < e.getData().size(); k++)
				{
					file.write((char*)&integral[i],sizeof(int));
				}
			}
			catch(Exception& e)
			{
				continue;
			}
		}
		//write dtSpect
		for(size_t bin = 0; bin < m_tubes[i]->getDriftTimeSpectrum().getData().size(); bin++)
		{
			file.write((char*)&m_tubes[i]->getDriftTimeSpectrum()[bin],sizeof(uint32_t));
		}
	}

//	file.cd("dtSpect");
//	m_drifttimeSpect->Write();
//	file.cd("diffDriftTime");
//	_diffDtSpect->Write();
//	file.cd("rtRelation");
//	m_rtRelation->Write();

	file.close();
	cout << "Saving complete" << endl;
}

/**
 * Converts all event data stored in a binary file to the data types needed internally
 * The converted data is stored in DataSets for each drifttube.
 *
 * @brief Convert all data in the file to datatypes used internally
 *
 * @author Stefan Bieschke
 * @date July 17, 2017
 * @version Alpha 2.0
 *
 * @param filename relative path of the file containing raw data
 */
void Archive::convertAllEntries(const string filename)
{
	ifstream file(filename, ios::binary);
	FileParams par(readHeader(file));

	const size_t nEvents = par.nEvents;
	//cannot create std::arrays with a size that is not known at compiletime
	const size_t eventSize = 800;
	const size_t nTubes = par.nTubes;

	cout << "Beginning conversion:" << endl;
	cout << "Events: " << nEvents << endl << "tubes: " << nTubes << endl << "Bins per event: " << par.eventSize << endl;
	file.seekg(par.endOfHeader);

	for(size_t i = 0; i < nTubes; i++)
	{
		vector<unique_ptr<Event>> events(nEvents);
//		cout << "vector with events initialized. Size: " << events.size() << endl;
		for(size_t j = 0; j < events.size(); j++)
		{
			unique_ptr<array<uint16_t,eventSize>> arr(new array<uint16_t,eventSize>);
//			cout << "Array for event #" << j << " initialized. Size: " << arr->size() << endl;
			for(size_t k = 0; k < arr->size(); k++)
			{
				uint16_t buffer;
				file.read((char*)&buffer,sizeof(uint16_t));
				(*arr)[k] = buffer;
			}
			unique_ptr<Event> e(new Event(j,move(arr)));
			//zero supression - if no valid drift time was found: reject (a.k.a store nullptr)
	#ifdef ZEROSUP
			if(e->getDriftTime() < 0)
			{
//				cout << "Rejected event #" << e->getEventNumber() << endl;
				e.reset();
			}
	#endif
			events[j] = move(e);
		}
		//TODO implement positions init
		unique_ptr<DataSet> set(new DataSet(events));

		m_tubes.push_back(unique_ptr<Drifttube>(new Drifttube(1,2,move(set))));
	}
	file.close();
	cout << "file closed" << endl;
}

/**
 * Parses the directory from the given String containing the full path to file.
 *
 * Example 1: For the string '/path/to/file.drift' result is '/path/to/'
 * Example 2: For the string './../../path/file.drift' result is './../../path/'
 *
 * @brief Directory parser
 *
 * @author Stefan Bieschke
 * @date July 17, 2017
 * @version Alpha 2.0
 *
 * @param filename string containing the full path to file
 *
 * @return string containing the relative or absolute path
 */
string Archive::parseDir(const string filename)
{
	size_t positionOfLastSlash = filename.find_last_of('/');
	string dir = filename.substr(0,positionOfLastSlash+1);
	return dir;
}

/**
 * Parses the file from the given string containing the full path to file.
 *
 * Example 1: For the string '/path/to/file.drift' result is 'file.drift'
 * Example 2: For the string './../../path/file.drift' result is 'file.drift'
 *
 * @brief Filename parser
 *
 * @author Stefan Bieschke
 * @date July 17, 2017
 * @version Alpha 2.0
 *
 * @param filename string containing the full path to file
 *
 * @return string containing the filename
 *
 */
string Archive::parseFile(string filename)
{
	size_t positionOfLastSlash = filename.find_last_of('/');
	string file = filename.substr(positionOfLastSlash + 1);
	return file;
}


//TODO comment
//TODO test
FileParams Archive::readHeader(ifstream& file)
{
	size_t nTubes, eventSize, nEvents;
	if(file.is_open())
	{
		file.seekg(0,ios::beg);
		file.read((char*)&nTubes,sizeof(size_t));
		file.read((char*)&nEvents,sizeof(size_t));
		file.read((char*)&eventSize,sizeof(size_t));
	}

	streampos pos = file.tellg();
	FileParams result;
	result.nTubes = nTubes;
	result.nEvents = nEvents;
	result.eventSize = eventSize;
	result.endOfHeader = pos;

	return result;
}
