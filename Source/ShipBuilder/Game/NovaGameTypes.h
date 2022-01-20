// Spaceship Builder - Gwennaël Arbona

#pragma once

#include "EngineMinimal.h"
#include "System/NovaAssetManager.h"
#include <cmath>
#include "NovaGameTypes.generated.h"

/*----------------------------------------------------
    General purpose types
----------------------------------------------------*/

/** Gameplay constants */
namespace ENovaConstants
{
constexpr int32 MaxContractsCount   = 5;
constexpr int32 MaxPlayerCount      = 3;
constexpr int32 MaxCompartmentCount = 10;
constexpr int32 MaxModuleCount      = 5;
constexpr int32 MaxEquipmentCount   = 8;

constexpr double ResaleDepreciation        = 0.5;
constexpr float  SkirtCapacityMultiplier   = 1.1f;
constexpr int32  MaxTrajectoryDurationDays = 45;

const FDateTime ZeroTime     = FDateTime(2340, 2, 10, 8);
const FString   DefaultLevel = TEXT("Space");

constexpr float  StandardGravity         = 9.807f;
constexpr double TrajectoryDistanceError = 0.1;

};    // namespace ENovaConstants

/** Serialization way */
enum class ENovaSerialize : uint8
{
	JsonToData,
	DataToJson
};

/*----------------------------------------------------
    Currency type
----------------------------------------------------*/

/** Currency type */
USTRUCT()
struct FNovaCredits
{
	GENERATED_BODY();

	FNovaCredits() : Value(0)
	{}

	FNovaCredits(int64 V) : Value(V)
	{}

	bool operator==(const FNovaCredits Other) const
	{
		return Value == Other.Value;
	}

	bool operator!=(const FNovaCredits Other) const
	{
		return !operator==(Other);
	}

	bool operator<(const FNovaCredits Other) const
	{
		return Value < Other.Value;
	}

	bool operator<=(const FNovaCredits Other) const
	{
		return Value <= Other.Value;
	}

	bool operator>(const FNovaCredits Other) const
	{
		return Value > Other.Value;
	}

	bool operator>=(const FNovaCredits Other) const
	{
		return Value >= Other.Value;
	}

	FNovaCredits operator+(const FNovaCredits Other) const
	{
		return FNovaCredits(Value + Other.Value);
	}

	FNovaCredits operator+=(const FNovaCredits Other)
	{
		Value += Other.Value;
		return *this;
	}

	FNovaCredits operator-() const
	{
		return FNovaCredits(-Value);
	}

	FNovaCredits operator-(const FNovaCredits Other) const
	{
		return FNovaCredits(Value - Other.Value);
	}

	FNovaCredits operator-=(const FNovaCredits Other)
	{
		Value -= Other.Value;
		return *this;
	}

	int64 GetValue() const
	{
		return Value;
	}

	UPROPERTY()
	int64 Value;
};

static FNovaCredits operator*(const FNovaCredits Credits, const float Value)
{
	return FNovaCredits(Credits.Value * Value);
}

static FNovaCredits operator*(const float Value, const FNovaCredits Credits)
{
	return FNovaCredits(std::round(Credits.Value * Value));
}

static FNovaCredits operator/(const FNovaCredits Credits, const float Value)
{
	return FNovaCredits(Credits.Value / Value);
}

static FNovaCredits operator/(const float Value, const FNovaCredits Credits)
{
	return FNovaCredits(Value / Credits.Value);
}

/*----------------------------------------------------
    Time type
----------------------------------------------------*/

/** Time type */
USTRUCT()
struct FNovaTime
{
	GENERATED_BODY();

	FNovaTime() : Minutes(0)
	{}

	FNovaTime(double M) : Minutes(M)
	{}

	bool IsValid() const
	{
		return Minutes >= 0;
	}

	bool operator==(const FNovaTime Other) const
	{
		return Minutes == Other.Minutes;
	}

	bool operator!=(const FNovaTime Other) const
	{
		return !operator==(Other);
	}

	bool operator<(const FNovaTime Other) const
	{
		return Minutes < Other.Minutes;
	}

	bool operator<=(const FNovaTime Other) const
	{
		return Minutes <= Other.Minutes;
	}

	bool operator>(const FNovaTime Other) const
	{
		return Minutes > Other.Minutes;
	}

	bool operator>=(const FNovaTime Other) const
	{
		return Minutes >= Other.Minutes;
	}

	FNovaTime operator+(const FNovaTime Other) const
	{
		return FNovaTime::FromMinutes(Minutes + Other.Minutes);
	}

	FNovaTime operator+=(const FNovaTime Other)
	{
		Minutes += Other.Minutes;
		return *this;
	}

	FNovaTime operator-() const
	{
		return FNovaTime::FromMinutes(-Minutes);
	}

	FNovaTime operator-(const FNovaTime Other) const
	{
		return FNovaTime::FromMinutes(Minutes - Other.Minutes);
	}

	FNovaTime operator-=(const FNovaTime Other)
	{
		Minutes -= Other.Minutes;
		return *this;
	}

	double operator/(const FNovaTime Other) const
	{
		return Minutes / Other.Minutes;
	}

	double AsDays() const
	{
		return Minutes / (60.0 * 24.0);
	}

	double AsHours() const
	{
		return Minutes / 60.0;
	}

	double AsMinutes() const
	{
		return Minutes;
	}

	double AsSeconds() const
	{
		return Minutes * 60.0;
	}

	static FNovaTime FromDays(double Value)
	{
		FNovaTime T;
		T.Minutes = Value * 24.0 * 60.0;
		return T;
	}

	static FNovaTime FromHours(double Value)
	{
		FNovaTime T;
		T.Minutes = Value * 60.0;
		return T;
	}

	static FNovaTime FromMinutes(double Value)
	{
		FNovaTime T;
		T.Minutes = Value;
		return T;
	}

	static FNovaTime FromSeconds(double Value)
	{
		FNovaTime T;
		T.Minutes = (Value / 60.0);
		return T;
	}

	UPROPERTY()
	double Minutes;
};

static FNovaTime operator*(const FNovaTime Time, const double Value)
{
	return FNovaTime::FromMinutes(Time.Minutes * Value);
}

static FNovaTime operator*(const double Value, const FNovaTime Time)
{
	return FNovaTime::FromMinutes(Time.Minutes * Value);
}

static FNovaTime operator/(const FNovaTime Time, const double Value)
{
	return FNovaTime::FromMinutes(Time.Minutes / Value);
}

static FNovaTime operator/(const double Value, const FNovaTime Time)
{
	return FNovaTime::FromMinutes(Value / Time.Minutes);
}

/*----------------------------------------------------
    Description types
----------------------------------------------------*/

/** Description of a tradable asset */
UCLASS(ClassGroup = (Nova))
class UNovaTradableAssetDescription : public UNovaAssetDescription
{
	GENERATED_BODY()

public:
	// Base price modulated by the economy
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	float BasePrice = 10;
};

/** Description of a tradable asset that has dedicated preview assets */
UCLASS(ClassGroup = (Nova))
class UNovaPreviableTradableAssetDescription : public UNovaTradableAssetDescription
{
	GENERATED_BODY()

public:
	virtual struct FNovaAssetPreviewSettings GetPreviewSettings() const override;

	virtual void ConfigurePreviewActor(class AActor* Actor) const override;

public:
#if WITH_EDITORONLY_DATA

	// Mesh to use to render the asset
	UPROPERTY(Category = Preview, EditDefaultsOnly)
	TSoftObjectPtr<class UStaticMesh> PreviewMesh;

	// Material to use to render the asset
	UPROPERTY(Category = Preview, EditDefaultsOnly)
	TSoftObjectPtr<class UMaterialInterface> PreviewMaterial;

	// Preview offset for the asset render, in units
	UPROPERTY(Category = Preview, EditDefaultsOnly)
	FVector PreviewOffset = FVector(25.0f, -25.0f, 0.0f);

	// Preview rotation for the asset render
	UPROPERTY(Category = Preview, EditDefaultsOnly)
	FRotator PreviewRotation = FRotator::ZeroRotator;

	// Preview scale for the asset render
	UPROPERTY(Category = Preview, EditDefaultsOnly)
	float PreviewScale = 5.0f;

#endif    // WITH_EDITORONLY_DATA
};

/** Trade price modifiers */
UENUM()
enum class ENovaPriceModifier : uint8
{
	Cheap,
	BelowAverage,
	Average,
	AboveAverage,
	Expensive
};

/*----------------------------------------------------
    Resources
----------------------------------------------------*/

/** Possible cargo types */
UENUM()
enum class ENovaResourceType : uint8
{
	General,
	Bulk,
	Liquid
};

/** Description of a resource */
UCLASS(ClassGroup = (Nova))
class UNovaResource : public UNovaPreviableTradableAssetDescription
{
	GENERATED_BODY()

public:
	/** Get a special empty resource */
	static const UNovaResource* GetEmpty();

	/** Get a special propellant resource */
	static const UNovaResource* GetPropellant();

public:
	// Type of cargo hold that will be required for this resource
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	ENovaResourceType Type = ENovaResourceType::Bulk;

	// Resource description
	UPROPERTY(Category = Properties, EditDefaultsOnly)
	FText Description;
};
