// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "particles/ParticleSystemComponent.h"
#include "Sound/SoundCue.h"

void AShotgun::Fire(const FVector& HitTarget)
{
	AWeapon::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn == nullptr)
	{
		return;
	}

	AController* InstigatorController = OwnerPawn->GetController();

	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");
	if (MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();

		// 计算霰弹枪子弹命中个数

		// 因为霰弹枪子弹散射的缘故，它有可能同时命中多个角色，我们也需要处理这种情况
		// 这里采用一个map结构处理，键为角色指针，值为子弹命中数量
		TMap<ABlasterCharacter*, uint32> HitMap;

		for (uint32 i = 0; i < NumberOfPellets; i++)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);

			ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(FireHit.GetActor());
			if (BlasterCharacter && HasAuthority() && InstigatorController)
			{
				if (HitMap.Contains(BlasterCharacter))
				{
					HitMap[BlasterCharacter]++;
				}
				else
				{
					HitMap.Emplace(BlasterCharacter, 1);
				}
			}

			if (ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(
					GetWorld(),
					ImpactParticles,
					FireHit.ImpactPoint,
					FireHit.ImpactNormal.Rotation()
				);
			}

			if (HitSound)
			{
				UGameplayStatics::PlaySoundAtLocation(
					this,
					HitSound,
					FireHit.ImpactPoint,
					0.5f,
					FMath::FRandRange(-.5f, .5f)
				);
			}

		}

		// 计算伤害并应用在角色身上
		for (auto HitPair : HitMap)
		{
			if (HitPair.Key && HasAuthority() && InstigatorController)
			{
				UGameplayStatics::ApplyDamage(
					HitPair.Key,
					Damage * HitPair.Value,
					InstigatorController,
					this,
					UDamageType::StaticClass()
				);
			}
		}
	}
}

void AShotgun::ShotgunTraceHitWithScatter(const FVector& HitTarget, TArray<FVector>& HitTargets)
{
	// 从武器的枪口位置开始追踪
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName("MuzzleFlash");

	if (MuzzleFlashSocket == nullptr) return;

	const FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
	const FVector TraceStart = SocketTransform.GetLocation();

	// 一个从跟踪起始位置指向命中目标的向量（归一化的向量）
	const FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	// 从轨迹起点到头部目标的某个位置
	const FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	
	for (uint32 i = 0; i < NumberOfPellets; i++)
	{
		FVector RandVec = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
		FVector EndLoc = SphereCenter + RandVec;
		FVector ToEndLoc = EndLoc - TraceStart;

		DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Magenta, true);
		DrawDebugSphere(GetWorld(), EndLoc, 4.0f, 12, FColor::Orange, true);
		DrawDebugLine(
			GetWorld(),
			TraceStart, FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()),
			FColor::Green,
			true);

		HitTargets.Add(FVector(TraceStart + ToEndLoc * TRACE_LENGTH / ToEndLoc.Size()));
	}
}
