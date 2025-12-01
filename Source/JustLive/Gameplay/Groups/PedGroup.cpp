#include "PedGroup.h"
#include "../Peds/Ped.h"

UPedGroup::UPedGroup()
{
	Leader = nullptr;
}

void UPedGroup::SetLeader(APed* NewLeader)
{
	if (!NewLeader) return;

	Leader = NewLeader;
	AddMember(NewLeader); // Ensure leader is a member
}

void UPedGroup::AddMember(APed* NewMember)
{
	if (NewMember && !Members.Contains(NewMember))
	{
		Members.Add(NewMember);
		NewMember->CurrentSquad = this;
	}
}

void UPedGroup::RemoveMember(APed* Member)
{
	if (Member && Members.Contains(Member))
	{
		Members.Remove(Member);
		Member->CurrentSquad = nullptr;

		// If Leader left, clear leader (Election logic needed elsewhere) or a better way is check who was added right after leader? He will be the next in command?
		if (Member == Leader)
		{
			Leader = nullptr;
		}
	}
}

bool UPedGroup::HasLeader() const
{
	return Leader != nullptr && IsValid(Leader);
}
