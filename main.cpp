#include "Common.h"
#include "Utils.h"
#include "SupportVectorMachine.h"
#include "Feature.h"
#include "PrecisionRecall.h"

void
printUsage(const char* execName)
{
	printf("Usage:\n");
	printf("\t%s TRAIN   <in:database> <feature type> <out:svm model>\n", execName);
	printf("\t%s PRED    <in:database> <in:svm model> [<out:prcurve.pr>] [<out:database.preds>]\n", execName);
	printf("\t%s PREDSL  <in:image.jpg> <in:svm model> <out:scoreimg.tga>\n", execName);
	printf("\t%s FEATVIZ <feature type> <in:img> <out:viz.tga>\n", execName);
	printf("\t%s SVMVIZ  <in:svm model> <out:viz.tga>\n", execName);
}

void
saveSVMModelAndFeatureType(const char* filename, const SupportVectorMachine& svm, std::string featureType)
{
	FILE* f = fopen(filename, "wb");
	if(f == NULL) {
		throw CError("Could not open file %s for writing", filename);
	}
	fprintf(f, "%s\n", featureType.c_str());
	svm.save(f);
	fclose(f);
}

void
loadSVMModelAndFeatureType(const char* filename, SupportVectorMachine& svm, std::string& featureType)
{
	FILE* f = fopen(filename, "rb");
	if(f == NULL) {
		throw CError("Could not open file %s for reading", filename);
	}

	char buff[100];
	fscanf(f, "%s", buff);
	featureType = std::string(buff);

	svm.load(f);

	fclose(f);
}

int
mainVizSVMModel(int argc, char** argv)
{
	if(argc < 2) {
		std::cerr << "ERROR: Incorrect number of arguments\n" << std::endl;
		printUsage(argv[0]);
		return EXIT_FAILURE;
	}

	const char* svmModelFName = argv[2];
	const char* outImgFName = argv[3];

	PRINT_MSG("Loading model from file");
	SupportVectorMachine svm;
	std::string featureType;
	loadSVMModelAndFeatureType(svmModelFName, svm, featureType);

	PRINT_MSG("Instantiating a feature extractor to render the SVM weights");
	FeatureExtractor* featExtractor = FeatureExtractorNew(featureType.c_str());

	Feature svmW = svm.getWeights();
	svmW -= svm.getBiasTerm() / (svmW.Shape().width * svmW.Shape().height * svmW.Shape().nBands);

	// Create two images, one for the positive weights and another
	// one for the negative weights
	Feature pos(svmW.Shape()), neg(svmW.Shape());
	pos.ClearPixels();
	neg.ClearPixels();

	for(int y = 0; y < pos.Shape().height; y++) {
		float* svmIt = (float*) svmW.PixelAddress(0, y, 0);
		float* p = (float*) pos.PixelAddress(0, y, 0);
		float* n = (float*) neg.PixelAddress(0, y, 0);

		for(int x = 0; x < pos.Shape().width * pos.Shape().nBands; x++, p++, n++, svmIt++) {
			if(*svmIt < 0) *n = fabs(*svmIt);
			else if(*svmIt > 0) *p = *svmIt;
		}
	}

	CByteImage negViz, posViz;
	posViz = featExtractor->render(pos, true);
	negViz = featExtractor->render(neg, true);

	// Put positive and negative weights images side by side in a color image.
	// Negative weights show up as red and positive weights show up as green.
	CByteImage negposViz(CShape(posViz.Shape().width * 2, posViz.Shape().height, 3));
	negposViz.ClearPixels();

	for(int y = 0; y < negposViz.Shape().height; y++) {
		uchar* n = (uchar*) negViz.PixelAddress(0, y, 0);
		uchar* np = (uchar*) negposViz.PixelAddress(0, y, 2);
		for(int x = 0; x < negViz.Shape().width; x++, n++, np+= 3) {
			*np = *n;
		}

		uchar* p = (uchar*) posViz.PixelAddress(0, y, 0);
		np = (uchar*) negposViz.PixelAddress(posViz.Shape().width, y, 1);
		for(int x = 0; x < negViz.Shape().width; x++, p++, np+= 3) {
			*np = *p;
		}
	}

	WriteFile(negposViz, outImgFName);

	// Cleanup
	delete featExtractor;

	return EXIT_SUCCESS;
}

int
mainVizFeature(int argc, char** argv)
{
	if(argc < 4) {
		std::cerr << "ERROR: Incorrect number of arguments\n" << std::endl;
		printUsage(argv[0]);
		return EXIT_FAILURE;
	}

	const char* featureType = argv[2];
	const char* imgFName = argv[3];
	const char* outImgFName = argv[4];

	CByteImage img;
	ReadFile(img, imgFName);

	FeatureExtractor* featExtractor = FeatureExtractorNew(featureType);

	Feature f = (*featExtractor)(img);

	CByteImage featViz;
	
	featViz = featExtractor->render(f);
	WriteFile(featViz, outImgFName);

	delete featExtractor;

	return EXIT_SUCCESS;
}

int
mainSVMTrain(int argc, char** argv)
{
	if(argc < 4) {
		std::cerr << "ERROR: Incorrect number of arguments\n" << std::endl;
		printUsage(argv[0]);
		return EXIT_FAILURE;
	}

	const char* dbFName = argv[2];
	const char* featureType = argv[3];
	const char* svmModelFName = argv[4];

	ImageDatabase db(dbFName);
	std::cout << db << std::endl;

	FeatureExtractor* featExtractor = FeatureExtractorNew(featureType);

	PRINT_MSG("Extracting features");
	FeatureSet features;
	(*featExtractor)(db, features);

	PRINT_MSG("Training SVM");
	SupportVectorMachine svm;
	svm.train(db.getLabels(), features);

	saveSVMModelAndFeatureType(svmModelFName, svm, featureType);

	delete featExtractor;
	return EXIT_SUCCESS;
}

int
mainSVMPredict(int argc, char** argv)
{
	if(argc < 3 || argc > 6) {
		std::cerr << "ERROR: Incorrect number of arguments\n" << std::endl;
		printUsage(argv[0]);
		return EXIT_FAILURE;
	}

	const char* dbFName = argv[2];
	const char* svmModelFName = argv[3];
	const char* prFName = (argc >= 4)?argv[4]:NULL;
	const char* predsFName = (argc >= 5)?argv[5]:NULL;

	std::string featureType;

	ImageDatabase db(dbFName);
	std::cout << db << std::endl;

	PRINT_MSG("Loading model from file");
	SupportVectorMachine svm;
	loadSVMModelAndFeatureType(svmModelFName, svm, featureType);

	PRINT_MSG("Extracting features");
	FeatureExtractor* featExtractor = FeatureExtractorNew(featureType.c_str());
	FeatureSet features;
	(*featExtractor)(db, features);

	PRINT_MSG("Predicting");
	std::vector<float> preds = svm.predict(features);

	PRINT_MSG("Computing Precision Recall Curve");
	PrecisionRecall pr(db.getLabels(), preds);
	PRINT_MSG("Average precision: " << pr.getAveragePrecision());

	if(prFName != NULL) pr.save(prFName);
	if(predsFName != NULL) {
		ImageDatabase predsDb(preds, db.getFilenames());
		predsDb.save(predsFName);
	}

	return EXIT_SUCCESS;
}

int
mainSVMPredictSlidinbWindow(int argc, char** argv)
{
	if(argc < 4) {
		std::cerr << "ERROR: Incorrect number of arguments\n" << std::endl;
		printUsage(argv[0]);
		return EXIT_FAILURE;
	}

	const char* imgFName = argv[2];
	const char* svmModelFName = argv[3];
	const char* outImgFName = argv[4];

	PRINT_MSG("Loading image");
	CByteImage img;
	ReadFile(img, imgFName);

	PRINT_MSG("Loading model from file");
	std::string featureType;
	SupportVectorMachine svm;
	loadSVMModelAndFeatureType(svmModelFName, svm, featureType);

	PRINT_MSG("Extracting features");
	FeatureExtractor* featExtractor = FeatureExtractorNew(featureType.c_str());
	FeatureSet features;
	Feature feat = (*featExtractor)(img);

	CByteImage viz = featExtractor->render(feat);
	WriteFile(viz, "viz.tga");

	CFloatImage scoreImg = svm.predictSlidingWindow(feat);

	float scoreMin, scoreMax;
	scoreImg.getRangeOfValues(scoreMin, scoreMax);
	scoreMin = 0;
	PRINT_EXPR(scoreMin);
	PRINT_EXPR(scoreMax);
	scoreImg -= scoreMin;
	scoreImg /= (scoreMax - scoreMin);

	// Composite score on top of original image
	CTransform3x3 s = CTransform3x3::Scale( double(scoreImg.Shape().width) / img.Shape().width, 
	                                        double(scoreImg.Shape().height) / img.Shape().height );

	CFloatImage scoreScaled(img.Shape().width, img.Shape().height, 1);
	WarpGlobal(scoreImg, scoreScaled, s, eWarpInterpNearest);

	CFloatImage overlayImg(img.Shape().width, img.Shape().height, 3);
	overlayImg.ClearPixels();
	for(int y = 0; y < img.Shape().height; y++) {
		float* ovIt = (float*) overlayImg.PixelAddress(0,y,0);
		float* scIt = (float*) scoreScaled.PixelAddress(0,y,0);
		uchar* imIt = (uchar*) img.PixelAddress(0,y,0);
		for(int x = 0; x < img.Shape().width; x++, ovIt+=3, imIt+=img.Shape().nBands, scIt++) {
			float imGray = (float(imIt[0]) + float(imIt[1]) + float(imIt[2])) / 255;

			// original image goes into red channel
			ovIt[2] = imGray / 2; 
			
			// Score
			if(*scIt < 0) continue;
			ovIt[0] += *scIt;
			ovIt[1] += *scIt;
			ovIt[2] += *scIt;
		}
	}
	WriteFile(overlayImg, outImgFName);

	return EXIT_SUCCESS;
}

int 
main(int argc, char **argv) 
{
	try {
		if(argc < 2) {
			printUsage(argv[0]);
			return EXIT_FAILURE;
		} else {
			if (strcasecmp(argv[1], "TRAIN") == 0) {
				return mainSVMTrain(argc, argv);
			} else if (strcasecmp(argv[1], "PRED") == 0) {
				return mainSVMPredict(argc, argv);
			} else if (strcasecmp(argv[1], "PREDSL") == 0) {
				return mainSVMPredictSlidinbWindow(argc, argv);
			} else if (strcasecmp(argv[1], "FEATVIZ") == 0) {
				return mainVizFeature(argc, argv);
			} else if (strcasecmp(argv[1], "SVMVIZ") == 0) {
				return mainVizSVMModel(argc, argv);
			} else {
				printUsage(argv[0]);
				return EXIT_FAILURE;
			}
		} 
	} catch(CError err) {
		std::cerr << "===================================================" << std::endl;
		std::cerr << "ERROR: Uncought exception:\n\t" << err.message << std::endl;
		std::cerr << "===================================================" << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
