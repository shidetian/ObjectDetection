#include "ImageDatabase.h"

using namespace std;

ImageDatabase::ImageDatabase():
_negativesCount(0), _positivesCount(0)
{
}

ImageDatabase::ImageDatabase(const char* dbFilename):
_negativesCount(0), _positivesCount(0)
{
	load(dbFilename);
}

ImageDatabase::ImageDatabase(const vector<float>& labels, const vector<string>& filenames):
_negativesCount(0), _positivesCount(0)
{
	_labels = labels;
	_filenames = filenames;

	for(vector<float>::iterator i = _labels.begin(); i != _labels.end(); i++) {
		if(*i > 0) _positivesCount++;
		else if(*i < 0) _negativesCount++;
	}
}

void 
ImageDatabase::load(const char *dbFilename) 
{
	_dbFilename = string(dbFilename);

	_negativesCount = 0;
	_positivesCount = 0; 
	
	ifstream f(dbFilename);
	if(!f.is_open()) {
		throw CError("Could not open file %s for reading", dbFilename);
	}

	int nItems;
	f >> nItems;

	assert(nItems > 0);
	_labels.resize(nItems);
	_filenames.resize(nItems);

	for(int i = 0; i < nItems; i++) {
		f >> _labels[i] >> _filenames[i];

		if(_labels[i] < 0) _negativesCount++;
		else if(_labels[i] > 0) _positivesCount++;
	}
}

void 
ImageDatabase::save(const char* dbFilename)
{
	ofstream f(dbFilename);
	if(!f.is_open()) {
		throw CError("Could not open file %s for writing", dbFilename);
	}

	f << _labels.size() << "\n";
	for(int i = 0; i < _labels.size(); i++) {
		f << _labels[i] << " " << _filenames[i] << "\n";
	}
}

std::ostream&
operator<<(std::ostream& s, const ImageDatabase& db)
{
	s << "DATABASE INFO\n"
	  << setw(20) << "Original filename:" << " " << db.getDatabaseFilename() << "\n"
	  << setw(20) << "Positives:" << setw(5) << right << db.getPositivesCount() << "\n"
	  << setw(20) << "Negaties:"   << setw(5) << right << db.getNegativesCount() << "\n"
	  << setw(20) << "Unlabeled:"  << setw(5) << right << db.getUnlabeledCount() << "\n"
	  << setw(20) << "Total:"      << setw(5) << right << db.getSize() << "\n";

	return s;
}