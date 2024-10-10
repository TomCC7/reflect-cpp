"""Use the github API to get lists of people who have contributed in various ways to reflect-cpp.

The script is borrowed from the
[pydantic project](https://github.com/pydantic/pydantic/blob/main/.github/actions/people/people.py),
and modified to work with the reflect-cpp repository.

Originally, the logic comes from
[FastAPI](https://github.com/tiangolo/fastapi/blob/master/.github/actions/people/app/main.py).
"""

import logging

import subprocess
import sys
from collections import Counter
from datetime import datetime, timedelta, timezone
from pathlib import Path
from typing import Any, Container, Dict, List, Set, Union

import requests
import yaml
from github import Github
from pydantic_settings import BaseSettings

from pydantic import BaseModel, SecretStr

github_graphql_url = "https://api.github.com/graphql"

discussions_query = """
query Q($after: String) {
  repository(name: "reflect-cpp", owner: "getml") {
    discussions(first: 100, after: $after) {
      edges {
        cursor
        node {
          number
          author {
            login
            avatarUrl
            url
          }
          title
          createdAt
          comments(first: 100) {
            nodes {
              createdAt
              author {
                login
                avatarUrl
                url
              }
              isAnswer
              replies(first: 10) {
                nodes {
                  createdAt
                  author {
                    login
                    avatarUrl
                    url
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}
"""

issues_query = """
query Q($after: String) {
  repository(name: "reflect-cpp", owner: "getml") {
    issues(first: 100, after: $after) {
      edges {
        cursor
        node {
          number
          author {
            login
            avatarUrl
            url
          }
          title
          createdAt
          state
          comments(first: 100) {
            nodes {
              createdAt
              author {
                login
                avatarUrl
                url
              }
            }
          }
        }
      }
    }
  }
}
"""

prs_query = """
query Q($after: String) {
  repository(name: "reflect-cpp", owner: "getml") {
    pullRequests(first: 100, after: $after) {
      edges {
        cursor
        node {
          number
          labels(first: 100) {
            nodes {
              name
            }
          }
          author {
            login
            avatarUrl
            url
          }
          title
          createdAt
          state
          comments(first: 100) {
            nodes {
              createdAt
              author {
                login
                avatarUrl
                url
              }
            }
          }
          reviews(first:100) {
            nodes {
              author {
                login
                avatarUrl
                url
              }
              state
            }
          }
        }
      }
    }
  }
}
"""


class Author(BaseModel):
    login: str
    avatarUrl: str
    url: str


# Issues and Discussions


class CommentsNode(BaseModel):
    createdAt: datetime
    author: Union[Author, None] = None


class Replies(BaseModel):
    nodes: List[CommentsNode]


class DiscussionsCommentsNode(CommentsNode):
    replies: Replies


class Comments(BaseModel):
    nodes: List[CommentsNode]


class DiscussionsComments(BaseModel):
    nodes: List[DiscussionsCommentsNode]


class IssuesNode(BaseModel):
    number: int
    author: Union[Author, None] = None
    title: str
    createdAt: datetime
    state: str
    comments: Comments


class DiscussionsNode(BaseModel):
    number: int
    author: Union[Author, None] = None
    title: str
    createdAt: datetime
    comments: DiscussionsComments


class IssuesEdge(BaseModel):
    cursor: str
    node: IssuesNode


class DiscussionsEdge(BaseModel):
    cursor: str
    node: DiscussionsNode


class Issues(BaseModel):
    edges: List[IssuesEdge]


class Discussions(BaseModel):
    edges: List[DiscussionsEdge]


class IssuesRepository(BaseModel):
    issues: Issues


class DiscussionsRepository(BaseModel):
    discussions: Discussions


class IssuesResponseData(BaseModel):
    repository: IssuesRepository


class DiscussionsResponseData(BaseModel):
    repository: DiscussionsRepository


class IssuesResponse(BaseModel):
    data: IssuesResponseData


class DiscussionsResponse(BaseModel):
    data: DiscussionsResponseData


# PRs


class LabelNode(BaseModel):
    name: str


class Labels(BaseModel):
    nodes: List[LabelNode]


class ReviewNode(BaseModel):
    author: Union[Author, None] = None
    state: str


class Reviews(BaseModel):
    nodes: List[ReviewNode]


class PullRequestNode(BaseModel):
    number: int
    labels: Labels
    author: Union[Author, None] = None
    title: str
    createdAt: datetime
    state: str
    comments: Comments
    reviews: Reviews


class PullRequestEdge(BaseModel):
    cursor: str
    node: PullRequestNode


class PullRequests(BaseModel):
    edges: List[PullRequestEdge]


class PRsRepository(BaseModel):
    pullRequests: PullRequests


class PRsResponseData(BaseModel):
    repository: PRsRepository


class PRsResponse(BaseModel):
    data: PRsResponseData


class Settings(BaseSettings):
    input_token: SecretStr
    github_repository: str = "getml/reflect-cpp"
    request_timeout: int = 30


def get_graphql_response(
    *,
    settings: Settings,
    query: str,
    after: Union[str, None] = None,
) -> Dict[str, Any]:
    headers = {"Authorization": f"token {settings.input_token.get_secret_value()}"}
    variables = {"after": after}
    response = requests.post(
        github_graphql_url,
        headers=headers,
        timeout=settings.request_timeout,
        json={"query": query, "variables": variables, "operationName": "Q"},
    )
    if response.status_code != 200:
        logging.error(f"Response was not 200, after: {after}")
        logging.error(response.text)
        raise RuntimeError(response.text)
    data = response.json()
    if "errors" in data:
        logging.error(f"Errors in response, after: {after}")
        logging.error(data["errors"])
        logging.error(response.text)
        raise RuntimeError(response.text)
    return data


def get_graphql_issue_edges(*, settings: Settings, after: Union[str, None] = None):
    data = get_graphql_response(settings=settings, query=issues_query, after=after)
    graphql_response = IssuesResponse.model_validate(data)
    return graphql_response.data.repository.issues.edges


def get_graphql_question_discussion_edges(
    *,
    settings: Settings,
    after: Union[str, None] = None,
):
    data = get_graphql_response(
        settings=settings,
        query=discussions_query,
        after=after,
    )
    graphql_response = DiscussionsResponse.model_validate(data)
    return graphql_response.data.repository.discussions.edges


def get_graphql_pr_edges(*, settings: Settings, after: Union[str, None] = None):
    data = get_graphql_response(settings=settings, query=prs_query, after=after)
    graphql_response = PRsResponse.model_validate(data)
    return graphql_response.data.repository.pullRequests.edges


def get_issues_experts(settings: Settings):
    issue_nodes: List[IssuesNode] = []
    issue_edges = get_graphql_issue_edges(settings=settings)

    while issue_edges:
        for edge in issue_edges:
            issue_nodes.append(edge.node)
        last_edge = issue_edges[-1]
        issue_edges = get_graphql_issue_edges(settings=settings, after=last_edge.cursor)

    commentors = Counter()
    last_month_commentors = Counter()
    authors: Dict[str, Author] = {}

    now = datetime.now(tz=timezone.utc)
    one_month_ago = now - timedelta(days=30)

    for issue in issue_nodes:
        issue_author_name = None
        if issue.author:
            authors[issue.author.login] = issue.author
            issue_author_name = issue.author.login
        issue_commentors = set()
        for comment in issue.comments.nodes:
            if comment.author:
                authors[comment.author.login] = comment.author
                if comment.author.login != issue_author_name:
                    issue_commentors.add(comment.author.login)
        for author_name in issue_commentors:
            commentors[author_name] += 1
            if issue.createdAt > one_month_ago:
                last_month_commentors[author_name] += 1

    return commentors, last_month_commentors, authors


def get_discussions_experts(settings: Settings):
    discussion_nodes: List[DiscussionsNode] = []
    discussion_edges = get_graphql_question_discussion_edges(settings=settings)

    while discussion_edges:
        for discussion_edge in discussion_edges:
            discussion_nodes.append(discussion_edge.node)
        last_edge = discussion_edges[-1]
        discussion_edges = get_graphql_question_discussion_edges(
            settings=settings, after=last_edge.cursor
        )

    commentors = Counter()
    last_month_commentors = Counter()
    authors: Dict[str, Author] = {}

    now = datetime.now(tz=timezone.utc)
    one_month_ago = now - timedelta(days=30)

    for discussion in discussion_nodes:
        discussion_author_name = None
        if discussion.author:
            authors[discussion.author.login] = discussion.author
            discussion_author_name = discussion.author.login
        discussion_commentors = set()
        for comment in discussion.comments.nodes:
            if comment.author:
                authors[comment.author.login] = comment.author
                if comment.author.login != discussion_author_name:
                    discussion_commentors.add(comment.author.login)
            for reply in comment.replies.nodes:
                if reply.author:
                    authors[reply.author.login] = reply.author
                    if reply.author.login != discussion_author_name:
                        discussion_commentors.add(reply.author.login)
        for author_name in discussion_commentors:
            commentors[author_name] += 1
            if discussion.createdAt > one_month_ago:
                last_month_commentors[author_name] += 1
    return commentors, last_month_commentors, authors


def get_experts(settings: Settings):
    # Migrated to only use GitHub Discussions
    # (
    #     issues_commentors,
    #     issues_last_month_commentors,
    #     issues_authors,
    # ) = get_issues_experts(settings=settings)
    (
        discussions_commentors,
        discussions_last_month_commentors,
        discussions_authors,
    ) = get_discussions_experts(settings=settings)
    # commentors = issues_commentors + discussions_commentors
    commentors = discussions_commentors
    # last_month_commentors = (
    #     issues_last_month_commentors + discussions_last_month_commentors
    # )
    last_month_commentors = discussions_last_month_commentors
    # authors = {**issues_authors, **discussions_authors}
    authors = {**discussions_authors}
    return commentors, last_month_commentors, authors


def get_contributors(settings: Settings):
    pr_nodes: List[PullRequestNode] = []
    pr_edges = get_graphql_pr_edges(settings=settings)

    while pr_edges:
        for edge in pr_edges:
            pr_nodes.append(edge.node)
        last_edge = pr_edges[-1]
        pr_edges = get_graphql_pr_edges(settings=settings, after=last_edge.cursor)

    contributors = Counter()
    commentors = Counter()
    reviewers = Counter()
    authors: Dict[str, Author] = {}

    for pr in pr_nodes:
        author_name = None
        if pr.author:
            authors[pr.author.login] = pr.author
            author_name = pr.author.login
        pr_commentors: Set[str] = set()
        pr_reviewers: Set[str] = set()
        for comment in pr.comments.nodes:
            if comment.author:
                authors[comment.author.login] = comment.author
                if comment.author.login == author_name:
                    continue
                pr_commentors.add(comment.author.login)
        for author_name in pr_commentors:
            commentors[author_name] += 1
        for review in pr.reviews.nodes:
            if review.author:
                authors[review.author.login] = review.author
                pr_reviewers.add(review.author.login)
        for reviewer in pr_reviewers:
            reviewers[reviewer] += 1
        if pr.state == "MERGED" and pr.author:
            contributors[pr.author.login] += 1
    return contributors, commentors, reviewers, authors


def get_top_users(
    *,
    counter: Counter,
    min_count: int,
    authors: Dict[str, Author],
    skip_users: Container[str],
):
    users = []
    for commentor, count in counter.most_common(50):
        if commentor in skip_users:
            continue
        if count >= min_count:
            author = authors[commentor]
            users.append(
                {
                    "login": commentor,
                    "count": count,
                    "avatarUrl": author.avatarUrl,
                    "url": author.url,
                }
            )
    return users


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    settings = Settings()  # type: ignore[reportCallIssue]
    logging.info(f"Using config: {settings.model_dump_json()}")
    g = Github(settings.input_token.get_secret_value())
    repo = g.get_repo(settings.github_repository)
    question_commentors, question_last_month_commentors, question_authors = get_experts(
        settings=settings
    )
    contributors, pr_commentors, reviewers, pr_authors = get_contributors(
        settings=settings
    )
    authors = {**question_authors, **pr_authors}
    maintainers_logins = {"liuzicheng1987", "Urfoex"}
    bot_names = {"github-actions"}
    maintainers = []
    for login in maintainers_logins:
        user = authors[login]
        maintainers.append(
            {
                "login": login,
                "answers": question_commentors[login],
                "prs": contributors[login],
                "avatarUrl": user.avatarUrl,
                "url": user.url,
            }
        )

    min_count_expert = 1
    min_count_last_month = 1
    min_count_contributor = 1
    min_count_reviewer = 1
    skip_users = maintainers_logins | bot_names
    experts = get_top_users(
        counter=question_commentors,
        min_count=min_count_expert,
        authors=authors,
        skip_users=skip_users,
    )
    last_month_active = get_top_users(
        counter=question_last_month_commentors,
        min_count=min_count_last_month,
        authors=authors,
        skip_users=skip_users,
    )
    top_contributors = get_top_users(
        counter=contributors,
        min_count=min_count_contributor,
        authors=authors,
        skip_users=skip_users,
    )
    top_reviewers = get_top_users(
        counter=reviewers,
        min_count=min_count_reviewer,
        authors=authors,
        skip_users=skip_users,
    )

    people = {
        "maintainers": maintainers,
        "experts": experts,
        "last_month_active": last_month_active,
        "top_contributors": top_contributors,
        "top_reviewers": top_reviewers,
    }
    people_path = Path("./docs/plugins/people.yml")
    if not people_path.exists():
        people_path.parent.mkdir(parents=True, exist_ok=True)
        people_path.touch()
    people_old_content = people_path.read_text(encoding="utf-8")
    new_people_content = yaml.dump(
        people, sort_keys=False, width=200, allow_unicode=True
    )
    if people_old_content == new_people_content:
        logging.info("The people data hasn't changed, finishing.")
        sys.exit(0)
    people_path.write_text(new_people_content, encoding="utf-8")

    logging.info("Setting up GitHub Actions git user")
    subprocess.run(["git", "config", "user.name", "github-actions"], check=True)
    subprocess.run(
        ["git", "config", "user.email", "github-actions@github.com"], check=True
    )

    branch_name = "people-update"
    logging.info(f"Creating a new branch {branch_name}")
    subprocess.run(["git", "checkout", "-b", branch_name], check=True)
    logging.info("Adding updated file")
    subprocess.run(["git", "add", str(people_path)], check=True)
    logging.info("Committing updated file")
    message = "Update People information in documentation"
    result = subprocess.run(["git", "commit", "-m", message], check=True)

    remote_branch_exists = subprocess.run(
        ["git", "ls-remote", "--heads", "origin", branch_name],
        capture_output=True,
        text=True,
    ).stdout.strip()

    if remote_branch_exists:
        logging.info("Pulling latest changes from remote branch")
        subprocess.run(["git", "pull", "--rebase", "origin", branch_name], check=True)
    else:
        logging.info(f"No remote branch named {branch_name} exists. Skipping pull.")

    logging.info("Pushing branch")
    subprocess.run(["git", "push", "origin", branch_name], check=True)
    logging.info("Creating PR")
    body = """
    This PR is created automatically by `update-people` GitHub workflow to update 
    the People information in the documentation. The workflow runs every first day 
    of a month.
    """
    pr = repo.create_pull(
        title=message,
        body=body,
        # base branch needs to be changed to main once
        # feature/docs_migration_mkdocs is merged into main
        base="feature/docs_migration_mkdocs",
        head=branch_name,
    )
    logging.info(f"Created PR: {pr.number}")
    logging.info("Finished")
