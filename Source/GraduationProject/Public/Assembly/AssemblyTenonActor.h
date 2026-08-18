#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AssemblyTenonActor.generated.h"

class AAssemblyMortiseActor;
class UAssemblyPartComponent;
class UStaticMeshComponent;

/** Movable assembly actor that forwards grab lifecycle notifications to the generic assembly component. */
UCLASS(Blueprintable)
class GRADUATIONPROJECT_API AAssemblyTenonActor : public AActor
{
	GENERATED_BODY()

public:
	AAssemblyTenonActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	/** Call after the external grab component has attached this actor's mesh. */
	UFUNCTION(BlueprintCallable, Category = "Assembly|Interaction")
	void NotifyGrabStarted();

	/** Call after the external grab component has released this actor's mesh. */
	UFUNCTION(BlueprintCallable, Category = "Assembly|Interaction")
	void NotifyGrabEnded();

	UFUNCTION(BlueprintCallable, Category = "Assembly")
	void ResetAssembly();

	UFUNCTION(BlueprintCallable, Category = "Assembly")
	void SetMortiseActor(AAssemblyMortiseActor* InMortiseActor);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Assembly")
	TObjectPtr<UStaticMeshComponent> TenonMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Assembly")
	TObjectPtr<UAssemblyPartComponent> AssemblyPart;

	/** Select the fixed mortise instance; its target anchor is assigned automatically. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Assembly|References")
	TObjectPtr<AAssemblyMortiseActor> MortiseActor;

private:
	void ConfigureTargetFromMortise();
};
