#include "Feature.h"

void
FeatureExtractor::operator()(const ImageDatabase& db, FeatureSet& featureSet) const
{
	int n = db.getSize();

	featureSet.resize(n);
	for(int i = 0; i < n; i++) {
		CByteImage img;
		ReadFile(img, db.getFilename(i).c_str());

		featureSet[i] = (*this)(img);
	}
}

CByteImage
FeatureExtractor::render(const Feature& f, bool normalizeFeat) const
{
	if(normalizeFeat) {
		CShape shape = f.Shape();
		Feature fAux(shape);

		float fMin, fMax;
		f.getRangeOfValues(fMin, fMax);

		for(int y = 0; y < shape.height; y++) {
			float* fIt = (float*) f.PixelAddress(0,y,0);
			float* fAuxIt = (float*) fAux.PixelAddress(0,y,0);

			for(int x = 0; x < shape.width * shape.nBands; x++, fAuxIt++, fIt++) {
				*fAuxIt = (*fIt) / fMax;
			}
		}

		return this->render(fAux);
	} else {
		return this->render(f);
	}
}

FeatureExtractor* 
FeatureExtractorNew(const char* featureType)
{
	if(strcasecmp(featureType, "tinyimg") == 0) return new TinyImageFeatureExtractor();
	if(strcasecmp(featureType, "hog") == 0) return new HOGFeatureExtractor();
	// Implement other features or call a feature extractor with a different set
	// of parameters by adding more calls here.
	if(strcasecmp(featureType, "myfeat1") == 0) throw CError("not implemented");
	if(strcasecmp(featureType, "myfeat2") == 0) throw CError("not implemented");
	if(strcasecmp(featureType, "myfeat3") == 0) throw CError("not implemented");
	else {
		throw CError("Unknown feature type: %s", featureType);
	}
}

// ============================================================================
// TinyImage
// ============================================================================

TinyImageFeatureExtractor::TinyImageFeatureExtractor(int targetWidth, int targetHeight):
_targetW(targetWidth), _targetH(targetHeight)
{
}

Feature 
TinyImageFeatureExtractor::operator()(const CByteImage& img_) const
{
	CFloatImage tinyImg(_targetW, _targetH, 3);

	/******** BEGIN TODO ********/
	// Compute tiny image feature, output should be _targetW by _targetH a grayscale image
	// Steps are:
	// 1) Convert image to grayscale (see convertRGB2GrayImage in Utils.h)
	// 2) Resize image to be _targetW by _targetH
	// 
	// Useful functions:
	// convertRGB2GrayImage, TypeConvert, WarpGlobal

	//printf("TODO: Feature.cpp:80\n"); exit(EXIT_FAILURE);
	int width = img_.Shape().width;
	int height = img_.Shape().height;
	CFloatImage tempImg(width, height, 3);
	TypeConvert(img_, tempImg);
	convertRGB2GrayImage(tempImg, tempImg);
	CTransform3x3 trans= CTransform3x3::Scale(_targetW/((float) width), _targetH/((float) height));
	WarpGlobal(tempImg, tinyImg, trans, EWarpInterpolationMode::eWarpInterpCubic);
	/******** END TODO ********/

	return tinyImg;
}

CByteImage 
TinyImageFeatureExtractor::render(const Feature& f) const
{
	CByteImage viz;
	TypeConvert(f, viz);
	return viz;
}

// ============================================================================
// HOG
// ============================================================================

static float derivKvals[3] = { -1, 0, 1};

HOGFeatureExtractor::HOGFeatureExtractor(int nAngularBins, bool unsignedGradients, int cellSize):
_nAngularBins(nAngularBins),
_unsignedGradients(unsignedGradients),
_cellSize(cellSize)
{
	_kernelDx.ReAllocate(CShape(3, 1, 1), derivKvals, false, 1);
	_kernelDx.origin[0] = 1;

	_kernelDy.ReAllocate(CShape(1, 3, 1), derivKvals, false, 1);
	_kernelDy.origin[0] = 1;

	// For visualization
	// A set of patches representing the bin orientations. When drawing a hog cell 
	// we multiply each patch by the hog bin value and add all contributions up to 
	// form the visual representation of one cell. Full HOG is achieved by stacking 
	// the viz for individual cells horizontally and vertically.
	_oriMarkers.resize(_nAngularBins);
	const int ms = 11;
	CShape markerShape(ms, ms, 1);

	// First patch is a horizontal line
	_oriMarkers[0].ReAllocate(markerShape, true);
	_oriMarkers[0].ClearPixels();
	for(int i = 1; i < ms - 1; i++) _oriMarkers[0].Pixel(/*floor(*/ ms/2 /*)*/, i, 0) = 1;

#if 0 // debug
	std::cout << "DEBUG:" << __FILE__ << ":" << __LINE__ << std::endl;
	for(int i = 0; i < ms; i++) {
		for(int j = 0; j < ms; j++) {
			std::cout << _oriMarkers[0].Pixel(j, i, 0) << " ";
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;

	char debugFName[2000];
	sprintf(debugFName, "/tmp/debug%03d.tga", 0);
	PRINT_EXPR(debugFName);
	WriteFile(_oriMarkers[0], debugFName);
#endif

	// The other patches are obtained by rotating the first one
	CTransform3x3 T = CTransform3x3::Translation((ms - 1) / 2.0, (ms - 1) / 2.0);
	for(int angBin = 1; angBin < _nAngularBins; angBin++) {
		double theta;
		if(unsignedGradients) theta = 180.0 * (double(angBin) / _nAngularBins);
		else theta = 360.0 * (double(angBin) / _nAngularBins);
		CTransform3x3 R  = T * CTransform3x3::Rotation(theta) * T.Inverse();

		_oriMarkers[angBin].ReAllocate(markerShape, true);
		_oriMarkers[angBin].ClearPixels();

		WarpGlobal(_oriMarkers[0], _oriMarkers[angBin], R, eWarpInterpLinear);

#if 0 // debug
		char debugFName[2000];
		sprintf(debugFName, "/tmp/debug%03d.tga", angBin);
		PRINT_EXPR(debugFName);
		WriteFile(_oriMarkers[angBin], debugFName);
#endif
	}
}

Feature 
HOGFeatureExtractor::operator()(const CByteImage& img_) const
{
	/******** BEGIN TODO ********/
	// Compute the Histogram of Oriented Gradients feature
	// Steps are:
	// 1) Compute gradients in x and y directions. We provide the 
	//    derivative kernel proposed in the paper in _kernelDx and
	//    _kernelDy.
	// 2) Compute gradient magnitude and orientation
	// 3) Add contribution each pixel to HOG cells whose
	//    support overlaps with pixel. Each cell has a support of size
	//    _cellSize and each histogram has _nAngularBins.
	// 4) Normalize HOG for each cell. One simple strategy that is
	//    is also used in the SIFT descriptor is to first threshold
	//    the bin values so that no bin value is larger than some
	//    threshold (we leave it up to you do find this value) and
	//    then re-normalize the histogram so that it has norm 1. A more 
	//    elaborate normalization scheme is proposed in Dalal & Triggs
	//    paper but we leave that as extra credit.
	// 
	// Useful functions:
	// convertRGB2GrayImage, TypeConvert, WarpGlobal, Convolve

	//printf("TODO: Feature.cpp:198\n"); exit(EXIT_FAILURE);
	
	//TODO: why not just use internal grayscale function?
	//CByteImage grayscale = ConvertToGray(img_);
	//CByteImage grayscale(img_.Shape());
	//convertRGB2GrayImage(img_, grayscale);
	CByteImage dx(img_.Shape());
	CByteImage dy(img_.Shape());

	Convolve(img_, dx, _kernelDx);
	Convolve(img_, dy, _kernelDy);

	//Third channel unused, first channel is mag, 2nd is orientation
	CFloatImage vals(img_.Shape()); 

	//Output feature image, one channel for each bin
	CFloatImage out(img_.Shape().width, img_.Shape().height, _nAngularBins);
	out.ClearPixels();
	for (int x=0; x<img_.Shape().width; x++){
		for (int y=0; y<img_.Shape().height; y++){
			//TODO: check that channel starts with 0
			vals.Pixel(x,y,0) = sqrt((float)dx.Pixel(x,y,0)*dx.Pixel(x,y,0)+dy.Pixel(x,y,0)*dx.Pixel(x,y,0));
			vals.Pixel(x,y,1) = atan2((float)dx.Pixel(x,y,0), dy.Pixel(x,y,0))*180/M_PI + 180;
			//take the channel with largest norm
			for (int c=1; c<3; c++){
				int temp = sqrt((float)dx.Pixel(x,y,c)*dx.Pixel(x,y,c)+dy.Pixel(x,y,c)*dx.Pixel(x,y,c));
				if (temp>vals.Pixel(x,y,0)){
					vals.Pixel(x,y,0) = temp;
					vals.Pixel(x,y,1) = max((float)((atan2((float)dx.Pixel(x,y,c), dy.Pixel(x,y,c))+M_PI)*180/M_PI), 0.0f);
					if(vals.Pixel(x,y,1)<0)
					{
						printf("OH NOES!!!!!");
					}
				}
			}			
		}
	}
	//Vote
	float bandWidth = 360 / (float)_nAngularBins;
	for (int x=0; x<img_.Shape().width; x++){
		for (int y=0; y<img_.Shape().height; y++){
			for (int c = max(0, x-_cellSize/2); c<min(img_.Shape().width, x+_cellSize/2); c++){
				for (int r = max(0, y-_cellSize/2); r<min(img_.Shape().height, y+_cellSize/2); r++){
					//TODO bilinear interpolation
					float test=vals.Pixel(c,r,1)/bandWidth;
					float testbin=floor(vals.Pixel(c,r,1)/bandWidth);
					float testval=vals.Pixel(c,r,0);
					if(testbin<0||testbin>_nAngularBins){
						printf("OH NOES!");
					}
					out.Pixel(x,y, max(0.0f,floor(vals.Pixel(c,r,1)/bandWidth)))+=vals.Pixel(c,r,0);
				}
			}
		}
	}

	//Normalization
	for (int x=0; x<img_.Shape().width; x++){
		for (int y=0; y<img_.Shape().height; y++){
			//Calc normalization
			float sqsum = 0.42; //this is a magic number
			for (int b=0; b<_nAngularBins; b++){
				sqsum+=out.Pixel(x,y,b)*out.Pixel(x,y,b);
			}
			sqsum = sqrt(sqsum);
			//Apply normalization
			for (int b=0; b<_nAngularBins; b++){
				out.Pixel(x,y,b)/=sqsum;
			}
		}
	}
	return out;
	/******** END TODO ********/

}

CByteImage 
HOGFeatureExtractor::render(const Feature& f) const
{
	CShape cellShape = _oriMarkers[0].Shape();
	CFloatImage hogImgF(CShape(cellShape.width * f.Shape().width, cellShape.height * f.Shape().height, 1));
	hogImgF.ClearPixels();

	float minBinValue, maxBinValue;
	f.getRangeOfValues(minBinValue, maxBinValue);

	// For every cell in the HOG
	for(int hi = 0; hi < f.Shape().height; hi++) {
		for(int hj = 0; hj < f.Shape().width; hj++) {

			// Now _oriMarkers, multiplying contribution by bin level
			for(int hc = 0; hc < _nAngularBins; hc++) {
				float v = f.Pixel(hj, hi, hc) / maxBinValue;
				for(int ci = 0; ci < cellShape.height; ci++) {
					float* cellIt = (float*) _oriMarkers[hc].PixelAddress(0, ci, 0);
					float* hogIt = (float*) hogImgF.PixelAddress(hj * cellShape.height, hi * cellShape.height + ci, 0);

					for(int cj = 0; cj < cellShape.width; cj++, hogIt++, cellIt++) {
						(*hogIt) += v * (*cellIt);
					}
				}
			}
		
		}
	}

	CByteImage hogImg;
	TypeConvert(hogImgF, hogImg);
	return hogImg;
}


