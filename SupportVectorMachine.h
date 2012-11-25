#ifndef SUPPORT_VECTOR_MACHINE_H
#define SUPPORT_VECTOR_MACHINE_H

#include "Common.h"
#include "Feature.h"
#include "ImageDatabase.h"

class SupportVectorMachine
{
private:
	struct svm_model* _model;
	svm_node* _data; // Have to keep this around if we want to save the model after training
	CShape _fVecShape; // Shape of feature vector

private:
	// De allocate memory
	void deinit();
	
public:
	SupportVectorMachine();

	// Loads SVM model from file with name modelFName
	SupportVectorMachine(const char* modelFName);
	~SupportVectorMachine();

	void train(const std::vector<float>& labels, const FeatureSet& fset, double C = 0.01);

	// Run classifier on feature, size of feature must match one used for
	// model training
	float predict(const Feature& feature) const;
	std::vector<float> predict(const FeatureSet& fset) const;

	// Get SVM weights in the shape of the original features
	Feature getWeights() const;
	double getBiasTerm() const;

	// Runs classifier at every location of feature feat, returns a
	// single channel image with classifier output at each location.
	CFloatImage predictSlidingWindow(const Feature& feat) const;

	// Loading and saving model to file
	void load(const char* filename);
	void load(FILE* fp);
	void save(const char* filename) const;
	void save(FILE* fp) const;
};

#endif