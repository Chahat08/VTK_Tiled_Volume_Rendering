// VTK_Tiled_Volume_Rendering.cpp : Defines the entry point for the application.
//

#include "VTK_Tiled_Volume_Rendering.h"

#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkContourValues.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkMetaImageReader.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkOpenGLGPUVolumeRayCastMapper.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkExternalOpenGLCamera.h>
#include <vtkMatrix4x4.h>

#include <iostream>

using namespace std;

float convertScale(float originalValue, float originalMin, float originalMax, float newMin, float newMax) {
	double originalRange = originalMax - originalMin;
	double newRange = newMax - newMin;
	return (((originalValue - originalMin) * newRange) / originalRange) + newMin;
}

int main(int argc, char* argv[])
{
	double iso1 = 500.0;
	double iso2 = 1150.0;

	if (argc < 2)
	{
		std::cout << "Usage: " << argv[0] << " file.mnd [iso1=500] [iso2=1150]"
			<< std::endl;
		std::cout << "e.g. FullHead.mhd 500 1150" << std::endl;
		return EXIT_FAILURE;
	}

	vtkNew<vtkMetaImageReader> reader;
	reader->SetFileName(argv[1]);

	vtkNew<vtkNamedColors> colors;

	vtkNew<vtkOpenGLGPUVolumeRayCastMapper> mapper;
	mapper->SetInputConnection(reader->GetOutputPort());
	mapper->AutoAdjustSampleDistancesOff();
	mapper->SetSampleDistance(0.5);
	mapper->SetBlendModeToIsoSurface();

	if (argc > 3)
	{
		iso1 = atof(argv[2]);
		iso2 = atof(argv[3]);
	}

	vtkNew<vtkColorTransferFunction> colorTransferFunction;
	colorTransferFunction->RemoveAllPoints();
	colorTransferFunction->AddRGBPoint(iso2,
		colors->GetColor3d("ivory").GetData()[0],
		colors->GetColor3d("ivory").GetData()[1],
		colors->GetColor3d("ivory").GetData()[2]);
	colorTransferFunction->AddRGBPoint(iso1,
		colors->GetColor3d("flesh").GetData()[0],
		colors->GetColor3d("flesh").GetData()[1],
		colors->GetColor3d("flesh").GetData()[2]);

	vtkNew<vtkPiecewiseFunction> scalarOpacity;
	scalarOpacity->AddPoint(iso1, .3);
	scalarOpacity->AddPoint(iso2, 0.6);

	vtkNew<vtkVolumeProperty> volumeProperty;
	volumeProperty->ShadeOn();
	volumeProperty->SetInterpolationTypeToLinear();
	volumeProperty->SetColor(colorTransferFunction);
	volumeProperty->SetScalarOpacity(scalarOpacity);

	vtkNew<vtkVolume> volume;
	volume->SetMapper(mapper);
	volume->SetProperty(volumeProperty);
	std::cout << volume->GetMinXBound() << " " << volume->GetMaxXBound() << " " << volume->GetMinYBound() << " " << volume->GetMaxYBound() << " " << volume->GetMinZBound() << " " << volume->GetMaxZBound() << "\n";
	//volume->SetPosition(0, 0, 5);

	vtkNew<vtkRenderer> renderer;
	renderer->AddVolume(volume);
	renderer->SetBackground(colors->GetColor3d("cornflower").GetData());
	renderer->ResetCamera();

	float sceneWidth = std::stof(argv[4]), sceneHeight = std::stof(argv[5]);
	float instanceWidth = std::stof(argv[6]), instanceHeight = std::stof(argv[7]);
	float instancePositionX = std::stof(argv[8]), instancePositionY = std::stof(argv[9]);
	float physicalHeight = std::stof(argv[10]), physicalDistance = std::stof(argv[11]);

	vtkNew<vtkRenderWindow> renderWindow;
	renderWindow->SetSize(instanceWidth, instanceHeight);
	renderWindow->SetPosition(instancePositionX, instancePositionY);
	renderWindow->BordersOff();
	renderWindow->AddRenderer(renderer);
	renderWindow->SetWindowName("RayCastIsosurface");

	vtkNew<vtkInteractorStyleTrackballCamera> style;

	vtkNew<vtkRenderWindowInteractor> interactor;
	interactor->SetRenderWindow(renderWindow);
	interactor->SetInteractorStyle(style);

	// Add some contour values to draw iso surfaces
	volumeProperty->GetIsoSurfaceValues()->SetValue(0, iso1);
	volumeProperty->GetIsoSurfaceValues()->SetValue(1, iso2);

	vtkNew<vtkExternalOpenGLCamera> camera;
	renderer->SetActiveCamera(camera);

	float aspectRatio = (float)sceneWidth / (float)sceneHeight;
	float fov = 2 * atan(physicalHeight / (2 * -physicalDistance));

	float near = std::max(physicalDistance - 1.0f, 0.1f);
	float far = 1000.0f;

	float halfHeight = tan(fov / 2) * near;
	float halfWidth = halfHeight * aspectRatio;

	float left = convertScale(instancePositionX, 0, sceneWidth, -halfWidth, halfWidth);
	float right = convertScale(instancePositionX + instanceWidth, 0, sceneWidth, -halfWidth, halfWidth);
	float top = convertScale(sceneHeight - instancePositionY, 0, sceneHeight, -halfHeight, halfHeight);
	float bottom = convertScale(sceneHeight - (instancePositionY + instanceHeight), 0, sceneHeight, -halfHeight, halfHeight);

	vtkNew<vtkMatrix4x4> projectionMatrix;
	projectionMatrix->Zero();
	projectionMatrix->SetElement(0, 0, (2 * near) / (right - left));
	projectionMatrix->SetElement(0, 2, (right + left) / (right - left));
	projectionMatrix->SetElement(1, 1, (2 * near) / (top - bottom));
	projectionMatrix->SetElement(1, 2, (top + bottom) / (top - bottom));
	projectionMatrix->SetElement(2, 2, -(far + near) / (far - near));
	projectionMatrix->SetElement(2, 3, -2 * far * near / (far - near));
	projectionMatrix->SetElement(3, 2, -1.0);

	camera->SetUseExplicitProjectionTransformMatrix(true);
	camera->SetExplicitProjectionTransformMatrix(projectionMatrix);
	double* volumePosition = volume->GetBounds();
	camera->SetPosition((volumePosition[0] + volumePosition[1]) / 2.0,
		(volumePosition[2] + volumePosition[3]) / 2.0,
		(volumePosition[4] + volumePosition[5]) / 2.0);

	renderWindow->Render();

	interactor->Start();

	return EXIT_SUCCESS;
}
