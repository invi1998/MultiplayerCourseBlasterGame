// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BlasterHUD.generated.h"

// 目前为止，BlasterHUD并不知道要绘制哪些纹理到屏幕上作为十字准心
// 所以这里提供一个结构体，容纳应该有的不同十字准心的纹理
USTRUCT(BlueprintType)
struct FHUDPackage
{
	// 添加主体生成宏 以便虚幻的反射系统可以将我们的FHUDPackage也加入到反射中
	GENERATED_BODY()

public:
	class UTexture2D* CrosshairsCenter;
	class UTexture2D* CrosshairsLeft;
	class UTexture2D* CrosshairsRight;
	class UTexture2D* CrosshairsTop;
	class UTexture2D* CrosshairsBottom;
	float CrosshairSpread;		// 该值用于控制十字准心应该分散多少
	FLinearColor CrosshairsColor;	// 十字准心的颜色
};

/**
 *
 */
UCLASS()
class BLASTER_API ABlasterHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<class UUserWidget> CharacterOverlayClass;

	UPROPERTY()
	class UCharacterOverlay* CharacterOverlay;

	UPROPERTY(EditAnywhere, Category = "Announcement")
	TSubclassOf<UUserWidget> AnnouncementClass;

	UPROPERTY(EditAnywhere, Category="ElimAnnoucement")
	TSubclassOf<class UElimAnnouncement> ElimAnnouncementClass;

	UPROPERTY()
	class UAnnouncement* Announcement;

	//UPROPERTY()
	//class UElimAnnouncement* ElimAnnouncement;

	void AddCharacterOverlay();
	void AddAnnouncement();
	void AddElimAnnouncement(FString Attacker, FString Victim);

protected:
	virtual void BeginPlay() override;

private:
	FHUDPackage HUDPackage;

	void DrawCrosshair(UTexture2D* Texture, FVector2D ViewportCenter, FVector2D Spread, FLinearColor Color);

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 16.f;

	UPROPERTY()
	class APlayerController* OwnerPlayerController;

	UPROPERTY(EditAnywhere, Category = "ElimAnnoucement")
	float ElimAnnouncementTime = 5.f;	// 击杀公告时间

	UFUNCTION()
	void ElimAnnouncementTimerFinished(UElimAnnouncement* MsgToRemove);	// 击杀公告时间结束

	UPROPERTY()
	TArray<UElimAnnouncement*> ElimMassages;	// 击杀公告消息

public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
};
