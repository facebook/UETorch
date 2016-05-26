/**
 * Copyright (c) 2015-present, Facebook, Inc.
 * All rights reserved.
 *
 * This source code is licensed under the BSD-style license found in the
 * LICENSE file in the root directory of this source tree. An additional grant
 * of patent rights can be found in the PATENTS file in the same directory.
 */

#include "UETorchPrivatePCH.h"
#include "TorchPluginComponent.h"
#include <type_traits>

class FUETorch : public IUETorch
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE(FUETorch, UETorch)

void FUETorch::StartupModule()
{
	printf("FUETorch Startup\n");
}

void FUETorch::ShutdownModule()
{
}

/*************************************************************************
 * UETorch helper functions
 * These are mapped to Lua through FFI in uetorch.lua
 *************************************************************************/


/**
 * Simulate a user input event (press or relese a key).
 *
 * @param key name of the key that should be pressed.
 *            A complete list of key binding names is available at
 *            https://wiki.unrealengine.com/List_of_Key/Gamepad_Input_Names
 * @param ControllerId the controller ID that input should be applied to, typically 0.
 * @param eventType the key event type:
 *        IE_PRESSED  - press the key
 *        IE_RELEASED - release the key
 */
 extern "C" void PressKey(const char *key, int ControllerId, int eventType) {
	auto fkey = FKey(key);
	auto ViewportClient = GEngine->GameViewport;

	ViewportClient->InputKey(ViewportClient->Viewport, ControllerId, fkey, (EInputEvent) eventType);
}

// SetTickDeltaBounds() requires a patch to Unreal Engine that adds MinDeltaSeconds and MaxDeltaSeconds
// to UWorld.
// This is an SFINAE check to see whether the patch has been applied.
struct UWorldHasMinDeltaSeconds
{
	struct Fallback { int MinDeltaSeconds; };
	struct Combined : UWorld, Fallback { };
	template<typename U, U> struct SFINAE;

	template<typename U> static char f(SFINAE<int Fallback::*, &U::MinDeltaSeconds>*);
	template<typename U> static int f(...);

	static bool const value = sizeof(f<Combined>(0)) == sizeof(int);
};


template<typename WorldT>
bool SetTickDeltaBoundsInternal(WorldT* World, float MinDeltaSeconds, float MaxDeltaSeconds, std::true_type)
{
	World->MinDeltaSeconds = MinDeltaSeconds;
	World->MaxDeltaSeconds = MaxDeltaSeconds;
	return true;
}

template<typename WorldT>
bool SetTickDeltaBoundsInternal(WorldT* World, float MinDeltaSeconds, float MaxDeltaSeconds, std::false_type)
{
	printf("You need to the patch file located at Engine/Plugins/UETorch/UnrealEngine.patch\n");
	printf("and rebuild Unreal Engine for SetTickDeltaBounds to work\n");
	return false;
}

/**
 * Set minimum and maximum delta time for each game engine loop 'tick'.
 * By default, UnrealEngine adjusts the tick length to correspond to real time,
 * so that the game proceeds in real time. Calling SetTickDeltaBounds with
 * MinDeltaSeconds == MaxDeltaSeconds fixes the tick rate of the game.
 * This is useful for running at faster-than-real-time, and also ensures a consistent
 * tick rate for reproducible simulation, fixed-fps screenshots, etc.
 *
 * @param _this the TorchPluginComponent
 * @param MinDeltaSeconds the minimum game time per tick
 * @param MaxDeltaSeconds the maximum game time per tick
 */
extern "C" bool SetTickDeltaBounds(UObject* _this, float MinDeltaSeconds, float MaxDeltaSeconds)
{
	UWorld* World = GEngine->GetWorldFromContextObject(_this);
	if(World == NULL) {
		printf("World null\n");
		return false;
	}
	return SetTickDeltaBoundsInternal(World, MinDeltaSeconds, MaxDeltaSeconds,
		std::integral_constant<bool, UWorldHasMinDeltaSeconds::value>());
}

typedef struct {
	int32 X;
	int32 Y;
} IntSize;

/**
 * @returns the size of the viewport, in pixels.
 */
extern "C" void GetViewportSize(IntSize* r)
{
	if(GEngine == NULL){
		printf("GEngine null\n");
		return;
	}
	if(GEngine->GameViewport == NULL){
		printf("GameViewport null\n");
		return;
	}
	if(GEngine->GameViewport->Viewport == NULL){
		printf("Viewport null\n");
		return;
	}
	FViewport* Viewport = GEngine->GameViewport->Viewport;
	auto size = Viewport->GetSizeXY();
	r->X = size.X;
	r->Y = size.Y;
}


/**
 * Capture a screenshot from this actor's viewport.
 *
 * @param size the size of the viewport.
 * @param data a float array of 3 * size->X * size->Y elements.
 *             This array is filled with the screenshot data in [Y,X,color] order.
 * @returns true if successful
 */
extern "C" bool CaptureScreenshot(IntSize* size, void* data)
{
	FlushRenderingCommands();

	if(GEngine == NULL){
		printf("GEngine null\n");
		return false;
	}
	if(GEngine->GameViewport == NULL){
		printf("GameViewport null\n");
		return false;
	}
	if(GEngine->GameViewport->Viewport == NULL){
		printf("Viewport null\n");
		return false;
	}

	FViewport* Viewport = GEngine->GameViewport->Viewport;
	TArray<FColor> Bitmap;

	if (size->X != Viewport->GetSizeXY().X || size->Y != Viewport->GetSizeXY().Y) {
		return false;
	}

	TSharedPtr<SWindow> WindowPtr = GEngine->GameViewport->GetWindow();

	bool bScreenshotSuccessful = false;

	if( WindowPtr.IsValid() && FSlateApplication::IsInitialized() )
	{
		FIntVector Size(size->X, size->Y, 0);
		TSharedRef<SWidget> WindowRef = WindowPtr.ToSharedRef();
		bScreenshotSuccessful = FSlateApplication::Get().TakeScreenshot(WindowRef, Bitmap, Size);
	}
	else
	{
		FIntRect Rect(0, 0, size->X, size->Y);
		bScreenshotSuccessful = GetViewportScreenShot(Viewport, Bitmap, Rect);
	}
	if(bScreenshotSuccessful)
	{
		float* values = (float*) data;
		for (const FColor& color : Bitmap) {
			*values++ = color.R / 255.0f;
		}
		for (const FColor& color : Bitmap) {
			*values++ = color.G / 255.0f;
		}
		for (const FColor& color : Bitmap) {
			*values++ = color.B / 255.0f;
		}
	}

	return bScreenshotSuccessful;
}

// Looks up the player's SceneView object
// modeled after APlayerController::GetHitResultAtScreenPosition
FSceneView* GetSceneView(APlayerController* PlayerController, UWorld* World) {
	if(GEngine == NULL){
		printf("GEngine null\n");
		return NULL;
	}
	if(GEngine->GameViewport == NULL){
		printf("GameViewport null\n");
		return NULL;
	}
	if(GEngine->GameViewport->Viewport == NULL){
		printf("Viewport null\n");
		return NULL;
	}

	auto Viewport = GEngine->GameViewport->Viewport;

	// Create a view family for the game viewport
	FSceneViewFamilyContext ViewFamily(
		FSceneViewFamily::ConstructionValues(
			Viewport,
			World->Scene,
			GEngine->GameViewport->EngineShowFlags )
		.SetRealtimeUpdate(true) );


	// Calculate a view where the player is to update the streaming from the players start location
	FVector ViewLocation;
	FRotator ViewRotation;
	ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(PlayerController->Player);
	if (LocalPlayer == NULL) {
		return NULL;
	}
	FSceneView* SceneView = LocalPlayer->CalcSceneView( &ViewFamily, /*out*/ ViewLocation, /*out*/ ViewRotation, Viewport );
	return SceneView;
}

// a variant of FSceneView::SafeDeprojectFVector2D that avoids InverseFast().
// Using InverseFast() sometimes leads to warning message spew from denormalized SceneView.ViewMatrix
void FSceneView__SafeDeprojectFVector2D(const FSceneView* SceneView, const FVector2D& ScreenPos, FVector& out_WorldOrigin, FVector& out_WorldDirection)
{
	const FMatrix InverseViewMatrix = SceneView->ViewMatrices.ViewMatrix.Inverse();
	const FMatrix InvProjectionMatrix = SceneView->ViewMatrices.GetInvProjMatrix();

	SceneView->DeprojectScreenToWorld(ScreenPos, SceneView->UnscaledViewRect, InverseViewMatrix, InvProjectionMatrix, out_WorldOrigin, out_WorldDirection);
}

// Looks up common UE objects necessary for capturing segmentation, etc.
bool InitCapture(UObject* _this, const IntSize* size, FViewport** pViewport, APlayerController** pPlayerController, UWorld** pWorld, FSceneView** pSceneView)
{
	FlushRenderingCommands();

	if(GEngine == NULL){
		printf("GEngine null\n");
		return false;
	}
	if(GEngine->GameViewport == NULL){
		printf("GameViewport null\n");
		return false;
	}
	if(GEngine->GameViewport->Viewport == NULL){
		printf("Viewport null\n");
		return false;
	}

	*pViewport = GEngine->GameViewport->Viewport;

	if (size->X != (*pViewport)->GetSizeXY().X || size->Y != (*pViewport)->GetSizeXY().Y) {
		printf("Wrong size\n");
		return false;
	}

	*pPlayerController = UGameplayStatics::GetPlayerController(_this, 0);
	if(*pPlayerController == NULL) {
		printf("PlayerController null\n");
		return false;
	}

	*pWorld = GEngine->GetWorldFromContextObject(_this);
	if(*pWorld == NULL) {
		printf("World null\n");
		return false;
	}

	*pSceneView = GetSceneView(*pPlayerController, *pWorld);
	if(*pSceneView == NULL) {
		printf("SceneView null\n");
		return false;
	}

	return true;
}

/**
 * Calculate the segmentation for a set of objects in the viewport image.
 * Each value in seg_data is the index in the objects array of the object at that pixel, or 0.
 *
 * @param _this the TorchPluginComponent
 * @param size the size of the viewport.
 * @param seg_data an int array of size->Y/stride * size->X/stride elements.
 *                 This array is filled with the segmentation data in [Y,X] order.
 *                 Each value (1..nObjects) corresponds to the index of the foreground object in the objects array,
 *                 or 0 if none of the objects is in the foreground at this pixel.
 * @param stride stride in pixels at which to compute the optical flow.
 * @param objects array of nObjects Actor* pointers which will be recorded in the segmentation mask
 * @param nObjects size of the objects array
 * @param verbose verbose output
 * @returns true if the optical flow capture was successful
 */
extern "C" bool CaptureSegmentation(UObject* _this, const IntSize* size, void* seg_data, int stride, const AActor** objects, int nObjects, bool verbose)
{
	FViewport* Viewport = nullptr;
	APlayerController* PlayerController = nullptr;
	UWorld* World = nullptr;
	FSceneView* SceneView = nullptr;

	bool bOk = InitCapture(_this, size, &Viewport, &PlayerController, &World, &SceneView);
	if(!bOk) {
		return false;
	}

	float HitResultTraceDistance = 100000.f;

	ECollisionChannel TraceChannel = ECollisionChannel::ECC_Visibility;
	bool bTraceComplex = false;
	FHitResult HitResult;
	int* seg_values = (int*) seg_data;

	if(verbose) {
		for(int i = 0; i < nObjects; i++) {
			printf("Object %d: %p\n", i, objects[i]);
		}
	}

	// Iterate over pixels
	FCollisionQueryParams CollisionQueryParams( "ClickableTrace", bTraceComplex );
	for (int y = 0; y < size->Y; y+=stride) {
		for (int x = 0; x < size->X; x+=stride) {

			FVector2D ScreenPosition(x, y);
			FVector WorldOrigin, WorldDirection;
			FSceneView__SafeDeprojectFVector2D(SceneView, ScreenPosition, WorldOrigin, WorldDirection);
			// Cast ray from pixel to find intersecting object
			bool bHit = World->LineTraceSingleByChannel(HitResult, WorldOrigin, WorldOrigin + WorldDirection * HitResultTraceDistance, TraceChannel, CollisionQueryParams);
			if (verbose) printf("E\n");
			AActor* Actor = NULL;
			*seg_values = 0; // no foreground object
			if(bHit) {
				Actor = HitResult.GetActor();
				if(Actor != NULL)
				{
					for (int i = 0; i < nObjects; i++) {
						if (objects[i] == Actor) {
							*seg_values = i+1;
							break;
						}
					}
				}
			}

			if(verbose) {
				printf("(%d, %d) Actor: %p Seg: %d bHit: %d\n",
					x, y, Actor, *seg_values, bHit);
			}
			seg_values++;
		}
	}
	return true;
}

/**
 * Like CaptureSegmentation, except that occluded objects are captured as well
 *
 * @param _this the TorchPluginComponent
 * @param size the size of the viewport.
 * @param seg_data an int array of size->Y/stride * size->X/stride * nObjects elements.
 *                 This array is filled with the segmentation data in [Y,X,object] order.
 *                 Each value (y,x,i) is 1 if object i is at pixel (y,x) (even if occluded), 0 otherwise.
 * @param stride stride in pixels at which to compute the optical flow.
 * @param objects array of nObjects Actor* pointers which will be recorded in the segmentation mask
 * @param nObjects size of the objects array
 * @param verbose verbose output
 * @returns true if the optical flow capture was successful
 */
extern "C" bool CaptureMasks(UObject* _this, const IntSize* size, void* seg_data, int stride, const AActor** objects, int nObjects, bool verbose)
{
	FViewport* Viewport = nullptr;
	APlayerController* PlayerController = nullptr;
	UWorld* World = nullptr;
	FSceneView* SceneView = nullptr;

	bool bOk = InitCapture(_this, size, &Viewport, &PlayerController, &World, &SceneView);
	if(!bOk) {
		return false;
	}

	float HitResultTraceDistance = 100000.f;

	ECollisionChannel TraceChannel = ECollisionChannel::ECC_Visibility;
	bool bTraceComplex = false;
	TArray<struct FHitResult> HitResults;
	char* seg_values = (char*) seg_data;

	if(verbose) {
		for(int i = 0; i < nObjects; i++) {
			printf("Object %d: %p\n", i, objects[i]);
		}
	}

	AActor* Actor = nullptr;
	FCollisionQueryParams CollisionQueryParams( "ClickableTrace", bTraceComplex );
	for (int y = 0; y < size->Y; y+=stride) {
		for (int x = 0; x < size->X; x+=stride) {

			FVector2D ScreenPosition(x, y);

			FVector WorldOrigin, WorldDirection;
			FSceneView__SafeDeprojectFVector2D(SceneView, ScreenPosition, WorldOrigin, WorldDirection);

			HitResults.Reset();

			// LineTraceMultiByChannel stops recording hits after it sees a blocking hit in the trace channel,
			// so we don't want any objects to generate blocking hits.
			//
			// By setting collision channel to 0 (default) and CollisionResponseParams to ECR_Overlap,
			// I cause all objects to generate non-blocking (Overlap) hit events
			//
			// Note: bHit is true only if a blocking hit is generated, so it should always be false here
			bool bHit = World->LineTraceMultiByChannel(HitResults, WorldOrigin, WorldOrigin + WorldDirection * HitResultTraceDistance, (ECollisionChannel) 0, CollisionQueryParams, FCollisionResponseParams(ECR_Overlap));

			for (int i = 0; i < nObjects; i++) {
				*seg_values = 0;
				for(int h = 0; h < HitResults.Num(); h++) {
					Actor = HitResults[h].GetActor();
					if (Actor == objects[i]) {
						if(verbose) {
							printf("  >> %d %d %d %d %p %p\n", x, y, i, h, Actor, objects[i]);
						}
						*seg_values = 1;
						break;
					}
				}
				seg_values++;
			}
		}
	}
	return true;
}

/**
 * Helper function for optical flow
 * Calculate dPixel / dScreen, i.e. how much the pixel coordinates change in dimension dim
 * per change in coordinates on the camera near plane (the 'screen')
 *
 * FIXME:
 * Figuring out the ScreenDx and ScreenDy vectors with projective geometry is 'tricky'
 * so I do numerical differentiation instead.
 * I'm a bit worried about float precision here.
 */
FVector getDPixelDScreen(const FVector2D &ScreenPosition, const int dim, const FSceneView *SceneView) {
	// centered difference
	FVector2D ScreenPositionP = ScreenPosition;
	ScreenPositionP[dim] += 1;
	FVector WorldOriginP, WorldDirectionP;
	FSceneView__SafeDeprojectFVector2D(SceneView, ScreenPositionP, WorldOriginP, WorldDirectionP);

	// centered difference
	FVector2D ScreenPositionM = ScreenPosition;
	ScreenPositionM[dim] -= 1;
	FVector WorldOriginM, WorldDirectionM;
	FSceneView__SafeDeprojectFVector2D(SceneView, ScreenPositionM, WorldOriginM, WorldDirectionM);

	FVector DScreenDPixel = (WorldOriginP - WorldOriginM) / 2.0f;
	FVector DPixelDScreen = DScreenDPixel / DScreenDPixel.SizeSquared();
	return DPixelDScreen;
}

FBodyInstance* GetBodyInstance(AActor* Actor) {
	auto SceneComponent = Actor->GetRootComponent();
	if(SceneComponent == NULL) return NULL;
	auto PrimitiveComponent = Cast<UPrimitiveComponent>(SceneComponent);
	if (PrimitiveComponent == NULL) return NULL;
	FBodyInstance* BodyInst = PrimitiveComponent->GetBodyInstance();
	return BodyInst;
}

/**
 * Calculate the optical flow at each pixel in the viewport.
 *
 * @param _this the TorchPluginComponent
 * @param size the size of the viewport.
 * @param flow_data a float array of size->Y/stride * size->X/stride * 2 elements.
 *                  This array is filled with optical flow vectors (of dim 2) in [Y,X] order.
 * @param rgb_data a float array of size->Y/stride * size->X/stride * 3 elements.
 *                  This array is filled with the optical flow RGB data in [Y,X,color] order.
 * @param maxFlow the scale for the RGB flow data. At flow=maxFlow, the RGB output is saturated at 1.
 * @param stride stride in pixels at which to compute the optical flow.
 * @param verbose verbose output
 * @returns true if the optical flow capture was successful
 */
extern "C" bool CaptureOpticalFlow(UObject* _this, const IntSize* size, void* flow_data, void* rgb_data, float maxFlow, int stride, bool verbose)
{
	FViewport* Viewport = nullptr;
	APlayerController* PlayerController = nullptr;
	UWorld* World = nullptr;
	FSceneView* SceneView = nullptr;

	bool bOk = InitCapture(_this, size, &Viewport, &PlayerController, &World, &SceneView);
	if(!bOk) {
		return false;
	}

	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(_this, 0);
	if(PlayerCharacter == NULL) {
		printf("PlayerCharacter null\n");
		return false;
	}

	float HitResultTraceDistance = 100000.f;

	// 1. Get player/camera info
	FVector PlayerLoc  = PlayerCharacter->GetActorLocation();
	FRotator PlayerRot = PlayerController->GetControlRotation();
	FVector PlayerVel = PlayerCharacter->GetVelocity();
	FRotationMatrix PlayerRotMat(PlayerRot);

	FVector PlayerF = PlayerRotMat.GetScaledAxis( EAxis::X );
	PlayerF.Normalize();

	FBodyInstance* PlayerBodyInst = GetBodyInstance(PlayerCharacter);
	PlayerBodyInst->SetAngularVelocity(FVector(0,0,0), false); // FIXME

	// 2. Iterate over pixels
	ECollisionChannel TraceChannel = ECollisionChannel::ECC_Visibility; // FIXME?
	bool bTraceComplex = false; // FIXME?
	FHitResult HitResult;
	float* flow_values = (float*) flow_data;
	float* rgb_values  = (float*) rgb_data;
	FCollisionQueryParams CollisionQueryParams( "ClickableTrace", bTraceComplex );
	for (int y = 0; y < size->Y; y+=stride) {
		for (int x = 0; x < size->X; x+=stride) {

			FVector2D ScreenPosition(x, y);


			FVector WorldOrigin, WorldDirection;
			FSceneView__SafeDeprojectFVector2D(SceneView, ScreenPosition, WorldOrigin, WorldDirection);

			// 3. Calculate dPixel / dScreen, i.e. the pixel movement resulting from a movement in camera near plane
			FVector ScreenDx = getDPixelDScreen(ScreenPosition, 0, SceneView);
			FVector ScreenDy = getDPixelDScreen(ScreenPosition, 1, SceneView);

			// 4. Cast ray from pixel to find intersecting object
			bool bHit = World->LineTraceSingleByChannel(
				HitResult,
				WorldOrigin,
				WorldOrigin + WorldDirection * HitResultTraceDistance,
				TraceChannel,
				CollisionQueryParams);

			AActor* Actor = NULL;
			FVector CamVel, PointVel, Flow;

			if(bHit) {
				// 5. Get the location and velocity of the camera and the hit object
				const auto &HitLoc = HitResult.Location;
				Actor = HitResult.GetActor();
				FBodyInstance* ActorBodyInst = GetBodyInstance(Actor);
				if(ActorBodyInst != NULL)
				{
					PointVel = ActorBodyInst->GetUnrealWorldVelocityAtPoint(HitLoc);
				} else {
					printf("BodyInst null\n");
					PointVel = Actor->GetVelocity();
				}
				CamVel = PlayerBodyInst->GetUnrealWorldVelocityAtPoint(HitLoc);
				FVector RelVel = PointVel - CamVel;

				// 6. calculate the optical flow
				FVector HitLocRel = HitLoc - PlayerLoc;
				float DistToHit = FVector::DotProduct(HitLoc - PlayerLoc, PlayerF);
				FVector RelVelInCameraPlane = (RelVel - RelVel.ProjectOnTo(PlayerF)) / DistToHit;
				Flow.X = FVector::DotProduct(RelVelInCameraPlane, ScreenDx);
				Flow.Y = FVector::DotProduct(RelVelInCameraPlane, ScreenDy);
			} else {
				Flow.X  = 0;
				Flow.Y  = 0;
			}

			*flow_values++ = Flow.X;
			*flow_values++ = Flow.Y;

			// 7. Convert flow to RGB optical flow

			FVector PolarFlow;
			FMath::CartesianToPolar(Flow.X, Flow.Y, PolarFlow.X, PolarFlow.Y);
			float Hue = FMath::RadiansToDegrees(PolarFlow.Y);
			if(Hue < 0) Hue = Hue + 360.f;
			float Sat = FMath::Clamp(PolarFlow.X / maxFlow, 0.f, 1.f);

			FLinearColor HSV(Hue, Sat, 1);
			auto color = HSV.HSVToLinearRGB();

			*rgb_values++ = color.R;
			*rgb_values++ = color.G;
			*rgb_values++ = color.B;

			if(verbose) {
				printf("(%d, %d) PlayerRot: (%g, %g, %g) PointVel: (%g, %g, %g), CamVel: (%g, %g, %g) ScreenDx: (%g, %g, %g) ScreenDy: (%g, %g, %g) Flow: (%g, %g) PolarFlow: (%g, %g) HSV: (%g, %g, %g) RGB: (%g, %g, %g)\n",
					x, y,
					PlayerRot.Pitch, PlayerRot.Yaw, PlayerRot.Roll,
					PointVel.X, PointVel.Y, PointVel.Z,
					CamVel.X, CamVel.Y, CamVel.Z,
					ScreenDx.X, ScreenDx.Y, ScreenDx.Z,
					ScreenDy.X, ScreenDy.Y, ScreenDy.Z,
					Flow.X, Flow.Y,
					PolarFlow.X, PolarFlow.Y,
					HSV.R, HSV.G, HSV.B,
					color.R, color.G, color.B);
			}
		}
	}
	return true;
}


/**
 * Calculate the depth field.
 *
 * @param _this the TorchPluginComponent
 * @param size the size of the viewport.
 * @param data a float array of size->Y/stride * size->X/stride
 *             This array is filled with depth information.
 * @param stride stride in pixels at which to compute the optical flow.
 * @param verbose verbose output
 * @returns true if the optical flow capture was successful
 */
extern "C" bool CaptureDepthField(UObject* _this, const IntSize* size, void* data, int stride, bool verbose)
{
	FViewport* Viewport = nullptr;
	APlayerController* PlayerController = nullptr;
	UWorld* World = nullptr;
	FSceneView* SceneView = nullptr;

	bool bOk = InitCapture(_this, size, &Viewport, &PlayerController, &World, &SceneView);
	if(!bOk) {
		return false;
	}

	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(_this, 0);
	if(PlayerCharacter == NULL) {
		printf("PlayerCharacter null\n");
		return false;
	}

	float HitResultTraceDistance = 100000.f;

	FVector PlayerLoc  = PlayerCharacter->GetActorLocation();
	FRotator PlayerRot = PlayerController->GetControlRotation();
	FRotationMatrix PlayerRotMat(PlayerRot);

	FVector PlayerF = PlayerRotMat.GetScaledAxis( EAxis::X );
	PlayerF.Normalize();

	FBodyInstance* PlayerBodyInst = GetBodyInstance(PlayerCharacter);
	PlayerBodyInst->SetAngularVelocity(FVector(0,0,0), false); // FIXME

	ECollisionChannel TraceChannel = ECollisionChannel::ECC_Visibility; // FIXME?
	bool bTraceComplex = false; // FIXME?
	FHitResult HitResult;
	float* values = (float*) data;
	FCollisionQueryParams CollisionQueryParams( "ClickableTrace", bTraceComplex );
	for (int y = 0; y < size->Y; y+=stride) {
		for (int x = 0; x < size->X; x+=stride) {

			FVector2D ScreenPosition(x, y);

			FVector WorldOrigin, WorldDirection;
			FSceneView__SafeDeprojectFVector2D(SceneView, ScreenPosition, WorldOrigin, WorldDirection);

			bool bHit = World->LineTraceSingleByChannel(
				HitResult,
				WorldOrigin,
				WorldOrigin + WorldDirection * HitResultTraceDistance,
				TraceChannel,
				CollisionQueryParams);

			AActor* Actor = NULL;
			FVector CamVel, PointVel, Flow;

			if(bHit) {
				const auto &HitLoc = HitResult.Location;
				Actor = HitResult.GetActor();

				FVector HitLocRel = HitLoc - PlayerLoc;
				float DistToHit = FVector::DotProduct(HitLoc - PlayerLoc, PlayerF);
				*values++ = DistToHit;
			} else {
				*values++ = 0;
			}
		}
	}
	return true;
}

extern "C" bool SetActorPosition(AActor* object, double x, double y, double z) {
	return object->SetActorLocation(FVector(x,y,z), false);
}

extern "C" bool SetActorRotation(AActor* object, double pitch, double roll, double yaw) {
	return object->SetActorRotation(FRotator(pitch,roll,yaw));
}

extern "C" void SetActorVisible(AActor* object, bool visible) {
	object->SetActorHiddenInGame(!visible);
}

extern "C" bool SetActorVelocity(AActor* object, double x, double y, double z) {
	if(!object) {
		printf("object doesn't exist\n");
		return false;
	}
	UStaticMeshComponent *component = object->FindComponentByClass<UStaticMeshComponent>();
	if(!component) {
		printf("Object doesn't have StaticMeshComponent\n");
		return false;
	}
	component->SetPhysicsLinearVelocity(FVector(x,y,z));
	return true;
}

extern "C" bool SetMaterial(AActor* object, UMaterial* material) {
	if(!material) {
		printf("Material doesn't exist\n");
		return false;
	}
	UStaticMeshComponent *component = object->FindComponentByClass<UStaticMeshComponent>();
	if(!component) {
		printf("Object doesn't have StaticMeshComponent\n");
		return false;
	}
	component->SetMaterial(0, material);
	return true;
}
