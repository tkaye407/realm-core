////////////////////////////////////////////////////////////////////////////
//
// Copyright 2020 Realm Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or utilied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
////////////////////////////////////////////////////////////////////////////

#ifndef REMOTE_MONGO_COLLECTION_HPP
#define REMOTE_MONGO_COLLECTION_HPP

#include "sync/app_service_client.hpp"
#include <realm/util/optional.hpp>
#include <external/json/json.hpp>
#include <string>
#include <vector>

namespace realm {
namespace app {

class RemoteMongoCollection {
public:
    struct RemoteUpdateResult {
        /// The number of documents that matched the filter.
        int32_t matched_count;
        /// The number of documents modified.
        int32_t modified_count;
        /// The identifier of the inserted document if an upsert took place.
        util::Optional<ObjectId> upserted_id;
    };

    /// Options to use when executing a `find` command on a `RemoteMongoCollection`.
    struct RemoteFindOptions {
        /// The maximum number of documents to return.
        util::Optional<int64_t> limit;

        /// Limits the fields to return for all matching documents.
        util::Optional<bson::BsonDocument> projection_bson;

        /// The order in which to return matching documents.
        util::Optional<bson::BsonDocument> sort_bson;
    };

    /// Options to use when executing a `find_one_and_update`, `find_one_and_replace`,
    /// or `find_one_and_delete` command on a `remote_mongo_collection`.
    struct RemoteFindOneAndModifyOptions {
        /// Limits the fields to return for all matching documents.
        util::Optional<bson::BsonDocument> projection_bson;
        /// The order in which to return matching documents.
        util::Optional<bson::BsonDocument> sort_bson;
        /// Whether or not to perform an upsert, default is false
        /// (only available for find_one_and_replace and find_one_and_update)
        bool upsert = false;
        /// If this is true then the new document is returned,
        /// Otherwise the old document is returned (default)
        /// (only available for find_one_and_replace and find_one_and_update)
        bool return_new_document = false;

        void set_bson(bson::BsonDocument& bson)
        {
            if (upsert) {
                bson["upsert"] = true;
            }

            if (return_new_document) {
                bson["returnNewDocument"] = true;
            }

            if (projection_bson) {
                bson["projection"] = *projection_bson;
            }

            if (sort_bson) {
                bson["sort"] = *sort_bson;
            }
        }
    };

    ~RemoteMongoCollection() = default;
    RemoteMongoCollection(RemoteMongoCollection&&) = default;
    RemoteMongoCollection(const RemoteMongoCollection&) = default;
    RemoteMongoCollection& operator=(const RemoteMongoCollection& v) = default;
    RemoteMongoCollection& operator=(RemoteMongoCollection&&) = default;

    const std::string& name() const
    {
        return m_name;
    }

    const std::string& database_name() const
    {
        return m_database_name;
    }

    /// Finds the documents in this collection which match the provided filter.
    /// @param filter_bson A `Document` as bson that should match the query.
    /// @param options `RemoteFindOptions` to use when executing the command.
    /// @param completion_block The resulting bson array of documents or error if one occurs
    void find(const bson::BsonDocument& filter_bson, RemoteFindOptions options,
              std::function<void(util::Optional<bson::BsonArray>, util::Optional<AppError>)> completion_block);

    /// Finds the documents in this collection which match the provided filter.
    /// @param filter_bson A `Document` as bson that should match the query.
    /// @param completion_block The resulting bson array as a string or error if one occurs
    void find(const bson::BsonDocument& filter_bson,
              std::function<void(util::Optional<bson::BsonArray>, util::Optional<AppError>)> completion_block);

    /// Returns one document from a collection or view which matches the
    /// provided filter. If multiple documents satisfy the query, this method
    /// returns the first document according to the query's sort order or natural
    /// order.
    /// @param filter_bson A `Document` as bson that should match the query.
    /// @param options `RemoteFindOptions` to use when executing the command.
    /// @param completion_block The resulting bson or error if one occurs
    void find_one(const bson::BsonDocument& filter_bson, RemoteFindOptions options,
                  std::function<void(util::Optional<bson::BsonDocument>, util::Optional<AppError>)> completion_block);

    /// Returns one document from a collection or view which matches the
    /// provided filter. If multiple documents satisfy the query, this method
    /// returns the first document according to the query's sort order or natural
    /// order.
    /// @param filter_bson A `Document` as bson that should match the query.
    /// @param completion_block The resulting bson or error if one occurs
    void find_one(const bson::BsonDocument& filter_bson,
                  std::function<void(util::Optional<bson::BsonDocument>, util::Optional<AppError>)> completion_block);

    /// Runs an aggregation framework pipeline against this collection.
    /// @param pipeline A bson array made up of `Documents` containing the pipeline of aggregation operations to
    /// perform.
    /// @param completion_block The resulting bson array of documents or error if one occurs
    void aggregate(const bson::BsonArray& pipeline,
                   std::function<void(util::Optional<bson::BsonArray>, util::Optional<AppError>)> completion_block);

    /// Counts the number of documents in this collection matching the provided filter.
    /// @param filter_bson A `Document` as bson that should match the query.
    /// @param limit The max amount of documents to count
    /// @param completion_block Returns the count of the documents that matched the filter.
    void count(const bson::BsonDocument& filter_bson, int64_t limit,
               std::function<void(uint64_t, util::Optional<AppError>)> completion_block);

    /// Counts the number of documents in this collection matching the provided filter.
    /// @param filter_bson A `Document` as bson that should match the query.
    /// @param completion_block Returns the count of the documents that matched the filter.
    void count(const bson::BsonDocument& filter_bson,
               std::function<void(uint64_t, util::Optional<AppError>)> completion_block);

    /// Encodes the provided value to BSON and inserts it. If the value is missing an identifier, one will be
    /// generated for it.
    /// @param value_bson  A `Document` value to insert.
    /// @param completion_block The result of attempting to perform the insert. An Id will be returned for the
    /// inserted object on sucess
    void insert_one(const bson::BsonDocument& value_bson,
                    std::function<void(util::Optional<ObjectId>, util::Optional<AppError>)> completion_block);

    /// Encodes the provided values to BSON and inserts them. If any values are missing identifiers,
    /// they will be generated.
    /// @param documents  The `Document` values in a bson array to insert.
    /// @param completion_block The result of the insert, returns an array inserted document ids in order
    void insert_many(bson::BsonArray documents,
                     std::function<void(std::vector<ObjectId>, util::Optional<AppError>)> completion_block);

    /// Deletes a single matching document from the collection.
    /// @param filter_bson A `Document` as bson that should match the query.
    /// @param completion_block The result of performing the deletion. Returns the count of deleted objects
    void delete_one(const bson::BsonDocument& filter_bson,
                    std::function<void(uint64_t, util::Optional<AppError>)> completion_block);

    /// Deletes multiple documents
    /// @param filter_bson Document representing the match criteria
    /// @param completion_block The result of performing the deletion. Returns the count of the deletion
    void delete_many(const bson::BsonDocument& filter_bson,
                     std::function<void(uint64_t, util::Optional<AppError>)> completion_block);

    /// Updates a single document matching the provided filter in this collection.
    /// @param filter_bson  A bson `Document` representing the match criteria.
    /// @param update_bson  A bson `Document` representing the update to be applied to a matching document.
    /// @param upsert When true, creates a new document if no document matches the query.
    /// @param completion_block The result of the attempt to update a document.
    void update_one(const bson::BsonDocument& filter_bson, const bson::BsonDocument& update_bson, bool upsert,
                    std::function<void(RemoteUpdateResult, util::Optional<AppError>)> completion_block);

    /// Updates a single document matching the provided filter in this collection.
    /// @param filter_bson  A bson `Document` representing the match criteria.
    /// @param update_bson  A bson `Document` representing the update to be applied to a matching document.
    /// @param completion_block The result of the attempt to update a document.
    void update_one(const bson::BsonDocument& filter_bson, const bson::BsonDocument& update_bson,
                    std::function<void(RemoteUpdateResult, util::Optional<AppError>)> completion_block);

    /// Updates multiple documents matching the provided filter in this collection.
    /// @param filter_bson  A bson `Document` representing the match criteria.
    /// @param update_bson  A bson `Document` representing the update to be applied to a matching document.
    /// @param upsert When true, creates a new document if no document matches the query.
    /// @param completion_block The result of the attempt to update a document.
    void update_many(const bson::BsonDocument& filter_bson, const bson::BsonDocument& update_bson, bool upsert,
                     std::function<void(RemoteUpdateResult, util::Optional<AppError>)> completion_block);

    /// Updates multiple documents matching the provided filter in this collection.
    /// @param filter_bson  A bson `Document` representing the match criteria.
    /// @param update_bson  A bson `Document` representing the update to be applied to a matching document.
    /// @param completion_block The result of the attempt to update a document.
    void update_many(const bson::BsonDocument& filter_bson, const bson::BsonDocument& update_bson,
                     std::function<void(RemoteUpdateResult, util::Optional<AppError>)> completion_block);

    /// Updates a single document in a collection based on a query filter and
    /// returns the document in either its pre-update or post-update form. Unlike
    /// `update_one`, this action allows you to atomically find, update, and
    /// return a document with the same command. This avoids the risk of other
    /// update operations changing the document between separate find and update
    /// operations.
    /// @param filter_bson  A bson `Document` representing the match criteria.
    /// @param update_bson  A bson `Document` representing the update to be applied to a matching document.
    /// @param options Optional `RemoteFindOneAndModifyOptions` to use when executing the command.
    /// @param completion_block The result of the attempt to update a document.
    void find_one_and_update(
        const bson::BsonDocument& filter_bson, const bson::BsonDocument& update_bson,
        RemoteFindOneAndModifyOptions options,
        std::function<void(util::Optional<bson::BsonDocument>, util::Optional<AppError>)> completion_block);

    /// Updates a single document in a collection based on a query filter and
    /// returns the document in either its pre-update or post-update form. Unlike
    /// `update_one`, this action allows you to atomically find, update, and
    /// return a document with the same command. This avoids the risk of other
    /// update operations changing the document between separate find and update
    /// operations.
    /// @param filter_bson  A bson `Document` representing the match criteria.
    /// @param update_bson  A bson `Document` representing the update to be applied to a matching document.
    /// @param completion_block The result of the attempt to update a document.
    void find_one_and_update(
        const bson::BsonDocument& filter_bson, const bson::BsonDocument& update_bson,
        std::function<void(util::Optional<bson::BsonDocument>, util::Optional<AppError>)> completion_block);

    /// Overwrites a single document in a collection based on a query filter and
    /// returns the document in either its pre-replacement or post-replacement
    /// form. Unlike `update_one`, this action allows you to atomically find,
    /// replace, and return a document with the same command. This avoids the
    /// risk of other update operations changing the document between separate
    /// find and update operations.
    /// @param filter_bson  A `Document` that should match the query.
    /// @param replacement_bson  A `Document` describing the update.
    /// @param options Optional `RemoteFindOneAndModifyOptions` to use when executing the command.
    /// @param completion_block The result of the attempt to replace a document.
    void find_one_and_replace(
        const bson::BsonDocument& filter_bson, const bson::BsonDocument& replacement_bson,
        RemoteFindOneAndModifyOptions options,
        std::function<void(util::Optional<bson::BsonDocument>, util::Optional<AppError>)> completion_block);

    /// Overwrites a single document in a collection based on a query filter and
    /// returns the document in either its pre-replacement or post-replacement
    /// form. Unlike `update_one`, this action allows you to atomically find,
    /// replace, and return a document with the same command. This avoids the
    /// risk of other update operations changing the document between separate
    /// find and update operations.
    /// @param filter_bson  A `Document` that should match the query.
    /// @param replacement_bson  A `Document` describing the update.
    /// @param completion_block The result of the attempt to replace a document.
    void find_one_and_replace(
        const bson::BsonDocument& filter_bson, const bson::BsonDocument& replacement_bson,
        std::function<void(util::Optional<bson::BsonDocument>, util::Optional<AppError>)> completion_block);

    /// Removes a single document from a collection based on a query filter and
    /// returns a document with the same form as the document immediately before
    /// it was deleted. Unlike `delete_one`, this action allows you to atomically
    /// find and delete a document with the same command. This avoids the risk of
    /// other update operations changing the document between separate find and
    /// delete operations.
    /// @param filter_bson  A `Document` that should match the query.
    /// @param options Optional `RemoteFindOneAndModifyOptions` to use when executing the command.
    /// @param completion_block The result of the attempt to delete a document.
    void find_one_and_delete(
        const bson::BsonDocument& filter_bson, RemoteFindOneAndModifyOptions options,
        std::function<void(util::Optional<bson::BsonDocument>, util::Optional<AppError>)> completion_block);

    /// Removes a single document from a collection based on a query filter and
    /// returns a document with the same form as the document immediately before
    /// it was deleted. Unlike `delete_one`, this action allows you to atomically
    /// find and delete a document with the same command. This avoids the risk of
    /// other update operations changing the document between separate find and
    /// delete operations.
    /// @param filter_bson  A `Document` that should match the query.
    /// @param completion_block The result of the attempt to delete a document.
    void find_one_and_delete(
        const bson::BsonDocument& filter_bson,
        std::function<void(util::Optional<bson::BsonDocument>, util::Optional<AppError>)> completion_block);

    /*
     * SDKs should also support a watch method with the following 3 overloads:
     *      watch()
     *      watch(ids: List<Bson>)
     *      watch(filter: BsonDocument)
     *
     * In all cases, an asynchronous stream should be returned or a multi-shot
     * callback should be accepted depending on the idioms in your language.
     * The argument to send the server are a single BsonDocument, either empty
     * or with a single kv-pair for the argument name and value of the selected
     * overload.
     *
     * See the WatchStream class below for how to implement this stream.
     */

private:
    friend class RemoteMongoDatabase;

    RemoteMongoCollection(std::string name, std::string database_name, std::shared_ptr<AppServiceClient> service,
                          std::string service_name)
        : m_name(name)
        , m_database_name(database_name)
        , m_base_operation_args({{"database", database_name}, {"collection", name}})
        , m_service(service)
        , m_service_name(service_name)
    {
    }

    /// The name of this collection.
    std::string m_name;

    /// The name of the database containing this collection.
    std::string m_database_name;

    /// Returns a document of database name and collection name
    bson::BsonDocument m_base_operation_args;

    std::shared_ptr<AppServiceClient> m_service;

    std::string m_service_name;
};

/**
 * Simplifies the handling the stream for collection.watch() API.
 *
 * General pattern for languages with pull-based async generators (preferred):
 *    auto request = app.make_streaming_request("watch", ...);
 *    auto reply = await doHttpRequestUsingNativeLibs(request);
 *    if (reply.error)
 *        throw reply.error;
 *    auto ws = WatchStream();
 *    for await (chunk : reply.body) {
 *        ws.feedBuffer(chunk);
 *        while (ws.state == WatchStream::HAVE_EVENT) {
 *            yield ws.nextEvent();
 *        }
 *        if (ws.state == WatchStream::HAVE_ERROR)
 *            throw ws.error;
 *    }
 *
 * General pattern for languages with only push-based streams:
 *    auto request = app.make_streaming_request("watch", ...);
 *    doHttpRequestUsingNativeLibs(request, {
 *        .onError = [downstream](error) { downstream.onError(error); },
 *        .onHeadersDone = [downstream](reply) {
 *            if (reply.error)
 *                downstream.onError(error);
 *        },
 *        .onBodyChunk = [downstream, ws = WatchStream()](chunk) {
 *            ws.feedBuffer(chunk);
 *            while (ws.state == WatchStream::HAVE_EVENT) {
 *                downstream.nextEvent(ws.nextEvent());
 *            }
 *            if (ws.state == WatchStream::HAVE_ERROR)
 *                downstream.onError(ws.error);
 *        }
 *    });
 */
struct WatchStream {
    // NOTE: this is a fully processed event, not a single "data: foo" line!
    struct ServerSentEvent {
        std::string_view data;
        std::string_view eventType = "message";
    };

    // Call these when you have data, in whatever shape is easiest for your SDK to get.
    // Pick one, mixing and matching on a single instance isn't supported.
    // These can only be called in NEED_DATA state, which is the initial state.
    void feed_buffer(std::string_view); // May have multiple and/or partial lines.
    void feed_line(std::string_view);   // May include terminating CR and/or LF (not required).
    void feed_sse(ServerSentEvent);     // Only interested in "message" and "error" events. Others are ignored.

    // Call state() to see what to do next.
    enum State {
        NEED_DATA,  // Need to call one of the feed functions.
        HAVE_EVENT, // Call next_event() to consume an event.
        HAVE_ERROR, // Call error().
    };
    State state() const
    {
        return m_state;
    }

    // Consumes the returned event. If you used feed_buffer(), there may be another event or error after this one,
    // so you need to call state() again to see what to do next.
    bson::BsonDocument next_event()
    {
        REALM_ASSERT(m_state == HAVE_EVENT);
        auto out = std::move(m_next_event);
        m_state = NEED_DATA;
        advance_buffer_state();
        return out;
    }

    // Once this enters the error state, it stays that way. You should not feed any more data.
    const app::AppError& error() const
    {
        REALM_ASSERT(m_state == HAVE_ERROR);
        return *m_error;
    }

private:
    void advance_buffer_state();

    State m_state = NEED_DATA;
    util::Optional<app::AppError> m_error;
    bson::BsonDocument m_next_event;

    // Used by feed_buffer to construct lines
    std::string m_buffer;
    size_t m_buffer_offset = 0;

    // Used by feed_line for building the next SSE
    std::string m_event_type;
    std::string m_data_buffer;
};

} // namespace app
} // namespace realm

#endif /* remote_mongo_collection_h */