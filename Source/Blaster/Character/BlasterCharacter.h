// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BlasterCharacter.generated.h"

UCLASS()
class BLASTER_API ABlasterCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ABlasterCharacter();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostInitializeComponents() override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void MoveForward(float Value);

	void MoveRight(float Value);

	void Turn(float Value);

	void LookUp(float Value);

	void EquipButtonPressed();

private:
	// 添加弹簧臂和摄像机

	// 属性宏，让其在任何地方可见，想将相机，吊杆归为一类，叫做相机
	UPROPERTY(VisibleAnywhere, Category = Camera)
		class USpringArmComponent* CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = Camera)
		class UCameraComponent* FollowCamera;

	// 既然我们要从蓝图访问我们的部件，至少需要设置蓝图只读
	// 如果将私有变量设置蓝图只读，那么就需要允许私人访问，这意味着将这个变量暴露给蓝图
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
		class UWidgetComponent* OverheadWidget;

	// 武器类的前向声明
	// 我们想要复制这个变量，这意味着当它在服务器上发生变化时，我们想要同步在客户端上也进行更改，我们可以复制指向武器的指针
	UPROPERTY(ReplicatedUsing = OnRep_OverlappingWeapon)
		class AWeapon* OverlappingWeapon;

	UFUNCTION()
		void OnRep_OverlappingWeapon(AWeapon* LastWeapon);

	UPROPERTY(VisibleAnywhere)
		class UCombatComponent* Combat;

	UFUNCTION(Server, Reliable)
		void ServerEquipButtonPressed();

public:
	void SetOverlappingWeapon(AWeapon* Weapon);
	// 是否装备武器
	bool IsWeaponEquipped();
};
