// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Weapon.generated.h"

// 武器状态枚举，其枚举常量是无符号八位整数，同时这个也是一个蓝图类型
UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),		// 武器在世界上放置时的状态
	EWS_Equipped UMETA(DisplayName = "Equipped"),			// 武器被装备时的状态
	EWS_Dropped UMETA(DisplayName = "Dropped"),				// 武器被丢弃时的状态
	// 这些就是我们可以用于控制打开物理，打开碰撞，让武器反弹的地方

	EWS_MAX UMETA(DisplayName = "DefaultMax")				// 默认最大常量
};

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AWeapon();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	// 武器骨架网格
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
		USkeletalMeshComponent* WeaponMesh;

	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
		class USphereComponent* AreaSphere;

	UPROPERTY(VisibleAnywhere)
		EWeaponState WeaponState;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
