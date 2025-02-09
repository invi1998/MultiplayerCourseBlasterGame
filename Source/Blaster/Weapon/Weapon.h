// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
#include "Blaster/BlasterTypes/Team.h"
#include "Weapon.generated.h"

// 武器状态枚举，其枚举常量是无符号八位整数，同时这个也是一个蓝图类型
UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial UMETA(DisplayName = "Initial State"),		// 武器在世界上放置时的状态
	EWS_Equipped UMETA(DisplayName = "Equipped"),			// 武器被装备时的状态
	EWS_Dropped UMETA(DisplayName = "Dropped"),				// 武器被丢弃时的状态
	EWS_EquippedSecondary UMETA(DisplayName = "Equipped Secondary"),	// 武器被装备为次要武器时的状态
	// 这些就是我们可以用于控制打开物理，打开碰撞，让武器反弹的地方

	EWS_MAX UMETA(DisplayName = "DefaultMax")				// 默认最大常量
};

UENUM(BlueprintType)
enum class EFiringType : uint8
{
	EFT_HitScan UMETA(DisplayName = "Hit Scan Weapon"),		// 扫描
	EFT_Projectile UMETA(DisplayName = "Projectile Weapon"),			// 投射
	EFT_Shotgun UMETA(DisplayName = "Shotgun"),			// 霰弹枪

	EFT_MAX UMETA(DisplayName = "DefaultMax")				// 默认最大常量
};

UCLASS()
class BLASTER_API AWeapon : public AActor
{
	GENERATED_BODY()

public:
	/*
	 * 十字准心纹理
	 * 这里有几个变量，因为我想要控制左右上下以及中心的纹理
	 */
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D* CrosshairsCenter;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D* CrosshairsLeft;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D* CrosshairsRight;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D* CrosshairsTop;

	UPROPERTY(EditAnywhere, Category = Crosshairs)
	class UTexture2D* CrosshairsBottom;

	// 瞄准时的武器缩放 FOV
	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 30.f;

	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f;

	/*
	 * 自动开火
	 */

	UPROPERTY(EditAnywhere, Category = Combat)
	float FireDelay = 0.15f;

	// 标志是否是自动武器（自动开火的武器）
	UPROPERTY(EditAnywhere, Category = Combat)
	bool bAutomatic = true;

	UPROPERTY(EditAnywhere, Category = Weapon)
	class USoundCue* EquipSound;

	/*
	 * 启用或者禁用自定义深度
	 */
	void EnableCustomDepth(bool bEnable);

	// 武器是否被销毁当被丢弃时
	UPROPERTY(Replicated)
	bool bDestroyOnDrop = false;

	// 武器是否会被销毁
	UPROPERTY(Replicated)
	bool bDestroyWeapon = false;

	UPROPERTY(EditAnywhere)
	EFiringType FiringType;	// 射击类型

	/*
	 * 跟踪和散射
	 */
	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float DistanceToSphere = 800.f;		// 距离球体

	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	float SphereRadius = 75.0f;			// 球体半径


	UPROPERTY(EditAnywhere, Category = "Weapon Scatter")
	bool bUseScatter = false;	// 是否使用散射

public:
	// Sets default values for this actor's properties
	AWeapon();
	virtual void OnRep_Owner() override;
	virtual void Tick(float DeltaTime) override;
	void ShowPickupWidget(bool bShowWidget);
	void SetHUDAmmo();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Fire(const FVector& HitTarget);
	// 丢弃武器 / 死亡掉落武器
	virtual void Dropped();
	void AddAmmo(int32 Amount);

	FVector TraceEndWithScatter(const FVector& HitTarget) const;	// 跟踪结束位置


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void OnWeaponStateSet();

	virtual void OnEquipped();

	virtual void OnDropped();

	virtual void OnEquippedSecondary();

	// 武器的所有者
	UPROPERTY()
	class ABlasterCharacter* BlasterOwnerCharacter;

	// 武器的控制器
	UPROPERTY()
	class ABlasterPlayerController* BlasterOwnerController;

	// 在和武器的碰撞检测球体重叠的时候调用
	UFUNCTION()
	virtual void OnSphereOverlap(
			UPrimitiveComponent* OverlappedComponent,	// 原始组件
			AActor* OtherActor,
			UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex,
			bool bFromSweep,
			const FHitResult& SweepResult
		);

	// 在和武器的碰撞检测球体结束重叠的时候调用
	UFUNCTION()
	void OnSphereEndOverlap(
			UPrimitiveComponent* OverlappedComponent,	// 原始组件
			AActor* OtherActor,
			UPrimitiveComponent* OtherComp,
			int32 OtherBodyIndex
		);

	UPROPERTY(EditAnywhere)
	float Damage = 20.f;	// 武器伤害

	UPROPERTY(EditAnywhere)
	float HeadShotDamage = 50.f;	// 爆头伤害

	UPROPERTY(Replicated, EditAnywhere)
	bool bUseServerSideRewind = true;		// 是否使用服务器端倒带

	UFUNCTION()
	void OnPingTooHigh(bool bHighPing);		// 高延迟委托绑定函数

private:
	// 武器骨架网格
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;

	// 武器碰撞体
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class USphereComponent* AreaSphere;

	// 武器状态
	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;

	// 武器拾取的Widget组件
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	class UWidgetComponent* PickupWidget;

	// 武器状态同步
	UFUNCTION()
	void OnRep_WeaponState();

	// 武器开火动画
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	class UAnimationAsset* FireAnimation;

	// 武器准心
	UPROPERTY(EditAnywhere)
	TSubclassOf<class ACasing> CasingClass;

	// 武器子弹数量
	UPROPERTY(EditAnywhere)
	int32 Ammo;

	// 扣除弹药，检测武器是否有一个有效的所有者
	void SpendRound();

	UFUNCTION(Client, Reliable)
	void ClientUpdateAmmo(int32 ServerAmmo);

	UFUNCTION(Client, Reliable)
	void ClientAddAmmo(int32 Amount);

	// 未处理的服务器弹药请求数量
	// 在SpendRound()中增加，在ClientUpdateAmmo()中减少
	int32 Sequence = 0;

	// 武器的弹药容量
	UPROPERTY(EditAnywhere)
	int32 MagCapacity;

	// 武器类型
	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;

	UPROPERTY(EditAnywhere)
	ETeam Team;

public:
	// Called every frame

	void SetWeaponState(EWeaponState State);

	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }

	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }

	FORCEINLINE UWidgetComponent* GetPickupWidget() const { return PickupWidget; }

	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomedInterpSpeed() const { return ZoomInterpSpeed; }

	bool IsEmpty();
	bool IsFull();

	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }

	FORCEINLINE int32 GetAmmo() const { return Ammo; }
	FORCEINLINE int32 GetMagCapacity() const { return MagCapacity; }

	FORCEINLINE float GetDamage() const { return Damage; }
	FORCEINLINE float GetHeadShotDamage() const { return HeadShotDamage; }

	FORCEINLINE ETeam GetTeam() const { return Team; }
};
