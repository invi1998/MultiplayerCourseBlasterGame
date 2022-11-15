// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterCharacter.h"

#include "Blaster/BlasterComponents/CombatComponent.h"
#include "Blaster/Weapon/Weapon.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
ABlasterCharacter::ABlasterCharacter()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	// 将弹簧臂链接到网格上
	CameraBoom->SetupAttachment(GetMesh());
	// 设置相机臂长度
	CameraBoom->TargetArmLength = 600.f;
	// 把这个设置为true，这样当我们添加鼠标输入时，就可以随着控制器旋转相机吊杆。
	CameraBoom->bUsePawnControlRotation = true;

	// 前置相机
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	// 绑定相机臂
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// 因为有相机臂控制旋转，所以不需要设置旋转为true
	FollowCamera->bUsePawnControlRotation = false;

	// 不希望角色与控制器一起旋转
	bUseControllerRotationYaw = false;

	// 获取角色移动来访问角色移动组件
	GetCharacterMovement()->bOrientRotationToMovement = true;

	OverheadWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("OverheadWidget"));
	OverheadWidget->SetupAttachment(RootComponent);

	// 战斗组件
	Combat = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	Combat->SetIsReplicated(true);

	// 设置角色可蹲伏（也可以在UE编辑器中勾选）
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

	// 设置角色胶囊体不会阻挡摄像机，设置骨骼不会阻挡摄像机
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 这里就是我们需要注册要复制重叠武器变量的地方
	DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
}

void ABlasterCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	if (Combat)
	{
		Combat->Character = this;
	}
}

// Called when the game starts or when spawned
void ABlasterCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ABlasterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	AimOffset(DeltaTime);
}

// Called to bind functionality to input
void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ABlasterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ABlasterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &ABlasterCharacter::Turn);
	PlayerInputComponent->BindAxis("LookUp", this, &ABlasterCharacter::LookUp);

	// 按键绑定到操作执行函数
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Equip", IE_Pressed, this, &ABlasterCharacter::EquipButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &ABlasterCharacter::CrouchButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Pressed, this, &ABlasterCharacter::AimButtonPressed);
	PlayerInputComponent->BindAction("Aim", IE_Released, this, &ABlasterCharacter::AimButtonReleased);
}

void ABlasterCharacter::MoveForward(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::MoveRight(float Value)
{
	if (Controller != nullptr && Value != 0.f)
	{
		const FRotator YawRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);
		const FVector Direction(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y));
		AddMovementInput(Direction, Value);
	}
}

void ABlasterCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void ABlasterCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void ABlasterCharacter::EquipButtonPressed()
{
	if (Combat)
	{
		if (HasAuthority())
		{
			// 如果是服务端按下E，那就直接执行拾取武器的动作
			Combat->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			// 否则就是客户端，客户端需要发送RTC
			ServerEquipButtonPressed();
		}
	}
}

void ABlasterCharacter::CrouchButtonPressed()
{
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void ABlasterCharacter::AimButtonPressed()
{
	if (Combat)
	{
		Combat->SetAiming(true);
	}
}

void ABlasterCharacter::AimButtonReleased()
{
	if (Combat)
	{
		Combat->SetAiming(false);
	}
}

// 当我移动鼠标时，我希望角色的上半部分在站立状态下能按鼠标移动偏移量进行环顾四周的动作，
// 当偏移量超过一定限度（90°），我希望角色进行转向
void ABlasterCharacter::AimOffset(float DeltaTime)
{
	// 如果角色没有装备武器，那么就退出
	if (Combat && Combat->EquippedWeapon == nullptr) return;
	// 获取角色速度矢量
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	float Spead = Velocity.Size();
	bool bIsInAir = GetCharacterMovement()->IsFalling();

	// 当角色处于站立状态，并且没有跳跃（离地）
	if (Spead == 0.0f && !bIsInAir)
	{
		FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		bUseControllerRotationYaw = false;
	}

	// 跑动或者跳跃（离地）
	if (Spead > 0.f || bIsInAir)
	{
		// 当我们的角色跑步或者在空中时（当装备了武器的前提下），他会保存每一帧的旋转信息
		// 这个旋转信息将作为一个增量信息传递给上一个if使用
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		// 同时也应该将AO_Yaw置空清0，因为我们会在上面进行计算
		AO_Yaw = 0.f;
		// 同时一旦我们开始运动，我们就需要继续使用控制器旋转
		bUseControllerRotationYaw = true;
	}

	// 获取仰角并设置
	AO_Pitch = GetBaseAimRotation().Pitch;
	//if (HasAuthority() && !IsLocallyControlled())
	//{
	//	// 利用UE_LOG查看在服务端出现仰角错误bug的数据
	//	UE_LOG(LogTemp, Warning, TEXT("AO_Yaw : %f"), AO_Yaw);
	//	// 可以看到，我们的俯视角低头看地板的时候，我们服务端收到的数据确实270-360之间的数据，这不是我们设定中的我们的【-90,0）
	//}
	// !IsLocallyControlled() 这个判断就是确保不是在本机器上，非本机器就代表经过了RPC，数据被压缩过，需要进行还原映射
	if (AO_Pitch > 90.F && !IsLocallyControlled())
	{
		// 将仰角数据从 [270, 360) 映射到 [-90, 0)
		// 这个颜色将会修复因为在 CharacterMovementComponent中，这个组件在GetPackedAngles（获取仰角数据）
		// 这个函数为了将仰角数据（旋转数据）通过RPC网络传输时，为了减少宽带占用，将数据压缩到4字节，
		// 导致我们客户端的俯视角在服务端显示确实抬头的仰视角的bug
		// 在我们的程序编写中，我们习惯的角度控制是例如 -90到90，但是在虚幻引擎中，这个函数在获取这些旋转角度数据时，
		// 他会将其压缩为一个无符号的整形数
		// 这个压缩过程就是通过 FRotator::CompressAxisToShort(Yaw) , FRotator::CompressAxisToShort(Patch)
		// 这两个函数进行的压缩处理，函数原型如下
		///*FORCEINLINE uint32 UCharacterMovementComponent::PackYawAndPitchTo32(const float Yaw, const float Pitch)
		//{
		//	const uint32 YawShort = FRotator::CompressAxisToShort(Yaw);
		//	const uint32 PitchShort = FRotator::CompressAxisToShort(Pitch);
		//	const uint32 Rotation32 = (YawShort << 16) | PitchShort;
		//	return Rotation32;
		//}*/
		// 然后关于 这里主要的执行函数 CompressAxisToShort,原型如下
		//template<typename T>
		//FORCEINLINE uint16 TRotator<T>::CompressAxisToShort(T Angle)
		//{
		//	// map [0->360) to [0->65536) and mask off any winding
		//	return FMath::RoundToInt(Angle * (T)65536.f / (T)360.f) & 0xFFFF;
		//}
		// 从源码中可以看出来，这个函数这里所做的，就是将一个float的角度数据，他的数值范围是[0,360)，
		// (Angle * (T)65536.f / (T)360.f)这串计算得到的结果是一个介于 [0, 65536)之间的浮点值
		// RoundToInt则是进行四舍五入，将浮点数转为整形
		// 我们知道float数据是具有小数点精度的，这些精度信息是需要字节信息来进行存储的，
		// 通过将float数据mask映射到这个范围，并删除这些小数点，我们就得到了一个一个介于 [0, 65536)之间的整形值
		// 而 65535 这个值他的十六进制数正好是 0xFFFF,16bit位的最大整形数
		// 将前面的整形数与 0xFFFF 进行按位与 & 运算，得到的结果就是将传递来的浮点数按360°进行切割。
		// 意思就是，假设你传递进来361，那么在经过计算后，得到的结果其实就是1，这一轮的旋转其实和你旋转1°是一样的效果
		// 这个就是虚幻底层在调用RPC传输旋转角和俯仰角时，为了减少网络开销所做的优化

		// 现在，我们可以知道，我们的旋转被压缩为 [0,360)的格式，当它通过rpc调用后，他会被解压缩，还原回浮点数。
		// 这点就是这里还原，因为它还原使用的是[0,360）的数据，它丢失了我们的负数信息，所以这里我们需要做映射
		// [270, 360) -> [-90, 0)
		// map pitch from [270,260) to [-90,0)
		FVector2D InRange(270.f, 360.f);
		FVector2D OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(true);
	}

	if (LastWeapon)
	{
		LastWeapon->ShowPickupWidget(false);
	}
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
	if (Combat)
	{
		Combat->EquipWeapon(OverlappingWeapon);
	}
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	if (OverlappingWeapon)
	{
		OverlappingWeapon->ShowPickupWidget(false);
	}

	OverlappingWeapon = Weapon;

	if (IsLocallyControlled())
	{
		if (OverlappingWeapon)
		{
			OverlappingWeapon->ShowPickupWidget(true);
		}
	}
}

bool ABlasterCharacter::IsWeaponEquipped()
{
	return (Combat && Combat->EquippedWeapon);
}

bool ABlasterCharacter::IsAiming()
{
	return (Combat && Combat->bAiming);
}