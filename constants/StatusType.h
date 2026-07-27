//
// Created by Emmanuel Addo-Odame on 23/07/2026.
//

#ifndef DATASUPPORTAPI_STATUSTYPES_H
#define DATASUPPORTAPI_STATUSTYPES_H

namespace data_support::constants {

    enum StatusType {
        Draft = 0,
        Published = 1,
        Archived = 2,
        Deleted = 3,
        Enabled = 4,
        Disabled = 5,
        Upcoming = 6,
        Ongoing = 7,
        Completed = 8,
        Processing = 9,
        Scheduled = 10,
        Active = 11,
        InActive = 12,
    };

}
#endif //DATASUPPORTAPI_STATUSTYPES_H